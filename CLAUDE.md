# N0ARY Packet Radio BBS

## What This Is

A multi-daemon bulletin board system for amateur (ham) packet radio, written in C. Originally authored in the early 1990s for SunOS, now maintained on FreeBSD. Users connect via AX.25 radio links (through TNCs) or TCP/IP (telnet) to read/send messages, which are forwarded between BBSes across a mesh network.

## Building

```bash
# One-time setup: copy and edit site config
cp src/include/site_config.mk.example src/include/site_config.mk
# Set BBS_DIR, OS (FREEBSD=1), and optional features in site_config.mk

# Build (requires GNU Make)
cd src && gmake

# Debug build
cd src && BBS_DEBUG=1 gmake

# Install (to $BBS_DIR/bin)
cd src && gmake install

# Clean
cd src && gmake clean
```

Produces two static libraries (`libtools.a`, `libbbs.a`) and individual daemon/tool binaries prefixed `b_` (e.g. `b_bbs`, `b_bbsd`, `b_msgd`).

## Architecture

All IPC is TCP sockets with a line-oriented ASCII text protocol. BBSD is the supervisor; all other daemons register with it and receive configuration. BBSD listens on a fixed port (44510); all other daemon ports are dynamically assigned.

| Daemon | Directory | Role |
|--------|-----------|------|
| **BBSD** | `src/bbs/bbsd/` | Supervisor. Config distribution, health monitoring, session tracking, port locking. |
| **MSGD** | `src/bbs/msgd/` | Message storage, retrieval, aging, forwarding queue. Generates BIDs via `allocate_bid()`/`format_bid()` in `bid.c`. Checks duplicates via BIDD. |
| **BIDD** | `src/bbs/bidd/` | BID (Bulletin ID) duplicate detection. 65K-bucket hash table, periodic disk flush, age-out. |
| **USERD** | `src/bbs/userd/` | User accounts (callsign-based), profiles, permissions, last-seen tracking. |
| **WPD** | `src/bbs/wpd/` | White Pages directory. Callsign lookup, home-BBS resolution, global WP propagation. |
| **GATED** | `src/bbs/gated/` | Email-to-radio gateway. User-to-address mapping, access control, loop detection. |
| **LOGD** | `src/bbs/logd/` | Centralized activity logging for all sessions. |
| **TNCD** | `src/bbs/tncd/` | TNC (radio modem) interface. AX.25 protocol, KISS framing, PCAP logging. One instance per port. |
| **TCPD** | `src/bbs/tcpd/` | TCP/telnet access bridge. |

### Shared Libraries

- **bbslib** (`src/bbs/bbslib/`) — Client stubs for every daemon (`bbsd.c`, `msgd.c`, `userd.c`, `bidd.c`, `gated.c`, `logd.c`, `wpd.c`), plus message handling (`msg.c`), RFC 822 parsing (`rfc822.c`), and core data structures (`bbslib.h`).
- **tools** (`src/tools/`) — Socket I/O, SMTP client/server, daemon management, async event loop (`alEvent.c`), text utilities, syslog integration.

### BBS Client Process

`src/bbs/bbs/` — The interactive user-facing program. One instance per connected session. Handles message send/read/list/forward, file operations, door programs, sysop commands. Connects to daemons as needed.

### Offline Tools

- **PROCESS** (`src/bbs/process/`) — Reads forwarding queue, connects to remote BBSes, transfers messages.
- **GATEWAY** (`src/bbs/gateway/`) — Accepts inbound Internet mail, converts and routes via GATED/MSGD.

## Key Data Structures

### Message (`struct msg_dir_entry` in `src/bbs/bbslib/bbslib.h:310`)

```c
struct msg_dir_entry {
    long number, size, flags, visible;
    char list_text[256];
    char bid[LenBID];           // 20 chars; BID string (e.g. "32405_N0ARY")
    char sub[LenSUBJECT];      // 60 chars
    char passwd[LenPASSWD];
    struct msg_address { ... } to, from;
    time_t odate, kdate, cdate, edate, time2live, update_t;
    long read_cnt, fwd_cnt;
    struct text_line *header, *body, *read_by;
};
```

### Message Flags (`msg_dir_entry.flags`)

**Status:** `MsgActive` (0x01), `MsgOld` (0x02), `MsgHeld` (0x04), `MsgKilled` (0x08), `MsgImmune` (0x10), `MsgPending` (0x20)

**Type (mutually exclusive via `MsgTypeMask`):** `MsgPersonal` (0x40), `MsgBulletin` (0x80), `MsgNTS` (0x100), `MsgSecure` (0x200)

**Addressing:** `MsgLocal` (0x1000), `MsgCall` (0x2000), `MsgCategory` (0x4000), `MsgToNts` (0x8000), `MsgAtBbs` (0x01000000), `MsgAtDist` (0x02000000)

**Other:** `MsgBid` (0x40000000) = message carries a BID, `MsgSentFromHere` (0x40000), `MsgNoFwd` (0x80000), `MsgFrom` (0x20000000)

Use `SetMsgPersonal(m)`, `SetMsgBulletin(m)`, etc. to change type (clears `MsgTypeMask` first, appends RFC 822 type header). Test with `IsMsgPersonal(m)`, `IsMsgBulletin(m)`, etc.

## Message & BID Flow

### Local message creation
1. User issues `SP` (personal), `SB` (bulletin), `ST` (NTS traffic), or `SS` (secure).
2. `msg_snd()` in `msg_send.c` validates, assigns BID placeholder `"$"` for bulletins/distributions.
3. `get_message_body()` in `msg_body.c` collects body, parses R: routing headers.
4. `msg_SendMessage()` sends to MSGD via socket.
5. MSGD expands `"$"` → `allocate_bid()` → `format_bid()` (e.g. `32405_N0ARY`), checks via BIDD, registers.

### BID format (`src/bbs/msgd/bid.c`)
- Numbers 1–99,999: `NNNNN_CALL` (decimal, e.g. `32405_N0ARY`)
- Numbers 100,000–60,566,175: `BBBBB.CALL` (base-36, e.g. `ZZ9.N0ARY`)

### Forwarding
1. MSGD creates forwarding stamps in `Msgd_Fwd_Dir`: `TYPE.MSGNUM.ALIAS` (e.g. `P.00001.N0ARY`).
2. BBS `msg_forward()` in `msg_read.c` iterates the queue. For each message:
   - Builds proposal: `SP USER @ BBS.HLOC < FROM $BID\n` via `build_send_cmd()`.
   - Remote partner checks BID, responds OK (accept) or F> (reject).
   - If accepted: sends subject, R: routing header, body, `/EX` terminator.
3. After forwarding, personal/NTS messages with no remaining destinations are auto-killed.

### Incoming from partner BBS
- BID checked at two levels: `msg_snd()` checks via `bid_chk()`, MSGD re-checks during `send_message()`.
- MID (Message ID) system: synthetic `MID_<BBS>_<NUM>` generated from R: routing headers for duplicate detection of messages without BIDs.
- Duplicate bulletins are rejected. Duplicate personals are held for sysop review.

## Configuration

Runtime config is in a `Config` file read by BBSD (path set at build time). Other key config files in `src/bbs/etc/`:

| File | Purpose |
|------|---------|
| `Config` | Master daemon configuration (ports, paths, intervals) |
| `Route` | Message forwarding routing rules |
| `Translate` | Message field translation/rewrite rules |
| `Systems` | Peer BBS definitions for forwarding |
| `Personal` | Rules for treating categories as personal messages |
| `Reject` | Incoming message rejection criteria |
| `Group` | Bulletin category grouping |

## Code Conventions

- Pure C (C89-era style). No C++ or external frameworks.
- Heavy use of global variables for session state (`ImBBS`, `ImSysop`, `usercall`, `MyHelpLevel`, etc.).
- String handling is manual with fixed-size buffers. Uses `sprintf` in older code; newer code uses `snprintf`/`strlcpy`.
- Bitmask flags for message attributes (see `MsgPersonal`, `MsgBulletin`, etc.).
- Token-based command parser (`struct TOKEN`, see `src/bbs/bbs/tokens.h`).
- Help messages are embedded in comments (`/* HELPMSG ... */`) and extracted by `helpmsg/`.
- `NEXT(x)` macro used for linked list traversal (equivalent to `x = x->next`).
- `NextChar(p)` / `NextSpace(p)` macros for string parsing.
- Error handling via `error(N)` which looks up message number N.
- Platform: FreeBSD. Build with `gmake`. Compiler flags include `-Werror`.
