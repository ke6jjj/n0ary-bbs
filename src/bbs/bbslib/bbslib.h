#include <netinet/in.h>
#include <stdint.h>
#include <time.h>

#define FOREGROUND	(dbug_level & dbgFOREGROUND)
#define VERBOSE		(dbug_level & dbgVERBOSE && FOREGROUND)

#define KILL_INTERVAL	30
#define PING_INTERVAL	20

#define Prog_BBS		1
#define Prog_IMPORT		2
#define Prog_CALLBK     3
#define Prog_WP         4
#define Prog_REMOTE		5
#define Prog_FWD		6

#define PORT0		0x01
#define PORT1		0x02
#define PORT2		0x04
#define PORT3		0x08
#define PORT4		0x10
#define PORT5		0x20
#define PORT6		0x40
#define PORT7		0x80
#define PORT8		0x100
#define PORT9		0x200
#define PORT10		0x400
#define PORT11		0x800
#define PORT12		0x1000
#define PORT13		0x2000
#define PORT14		0x4000
#define PORT15		0x8000
#define PORT16		0x10000
#define PORT17		0x20000
#define PORT18		0x40000
#define PORT19		0x80000
#define PORT20		0x100000
#define PORT21		0x200000
#define PORT22		0x400000
#define PORT23		0x800000
#define PORT24		0x1000000
#define PORT25		0x2000000
#define PORT26		0x4000000
#define PORT27		0x8000000
#define PORT28		0x10000000
#define PORT29		0x20000000
#define PORT30		0x40000000
#define PORT31		0x80000000

#define tTNC		1
#define tPHONE		2
#define tCONSOLE	3
#define tSMTP		4
#define tTCP		5

#define pUNKNOWN	0
#define pLOCAL		1
#define pAX25		2
#define pTCPIPv4	3
#define pTCPIPv6	4

extern int
	batch_mode,
	PendingCmd,
	Program;

#define CapAsIs		0
#define CapFirst	1
#define AllLowerCase	2
#define AllUpperCase	3

#define isOK(p)		((*p == 'O') && (*(p+1) == 'K'))
#define isERROR(p)	(!isOK(p))

#define YES		0
#define NO		1
#define QUIT		2

#define	LenCALL		7
#define	LenSUBJECT	60
#define	LenFNAME	20
#define	LenLNAME	20
#define	LenQTH		30
#define	LenZIP		6
#define	LenHOME		7
#define	LenPHONE	15
#define	LenEQUIP	40
#define	LenCLUB		10
#define	LenMACRO	80
#define	LenPASSWD	9
#define	LenEMAIL	80
#define	LenFREQ		80
#define LenHLOC		40
#define LenBID		20
#define LenINCLUDE	150
#define LenREMOTEADDR	150

#define LenMAX		LenINCLUDE

#define PRINTF		user_printf
#define PRINT 		user_puts
#define GETS		user_gets

extern short bbscallsum;

extern int
	override_help_level,
	disable_help,
	no_prompt,
	preempt_init_more,
	more_cnt,
	maintenance_mode,
	bbs_fwd_mask;

char *
	get_call(char **str);

struct call_and_checksum {
	char	str[LenCALL];
	short	sum;
};

struct RemoteAddr_Local {};

struct RemoteAddr_AX25 {
	char callsign[LenCALL];
	char ssid;
};

struct RemoteAddr_TCPIPv4 {
	char ip[INET_ADDRSTRLEN];
	int port;
};

struct RemoteAddr_TCPIPv6 {
	char ip[INET6_ADDRSTRLEN]; /* Room for RFC5952 format */
	int port;
};
	
struct RemoteAddr {
	int addr_type; /* pUNKNOWN, pLOCAL, pAX25, etc */
	union {
		struct RemoteAddr_Local local;
		struct RemoteAddr_AX25  ax25;
		struct RemoteAddr_TCPIPv4 tcpipv4;
		struct RemoteAddr_TCPIPv6 tcpipv6;
	} u;
};
		
extern int Bbsd_Port;
extern char *Bbs_Host;

#define logOFF		0
#define logON		1
#define logONnCLR	2
extern int Logging;

#define dbgNONE			0
#define dbgVERBOSE		1
#define dbgQAMODE		2
#define dbgFOREGROUND	0x100
#define dbgIGNOREHOST	0x200
#define dbgNODAEMONS	0x400
#define dbgINHIBITMAIL	0x800
#define dbgTESTHOST	0x1000
extern int dbug_level;

#define CALLisOK		1
#define CALLisNAME		2
#define CALLisSUSPECT	3

#define tEND		0
#define tSTRING		1
#define tINT		2
#define tDIRECTORY	3
#define tFILE		4
#define tTIME		5
#define tCOMMENT	20

struct ConfigurationList {
	char *token;
	int	type;
	void *ptr;
};

struct BbsdCommands {
	int token;
	char *key;
};

extern struct BbsdCommands BbsdCmds[];

#define bALERT		1
#define bCHAT		(bALERT+1)
#define bCHECK		(bCHAT+1)
#define bLOCK		(bCHECK+1)
#define bLOGIN		(bLOCK+1)
#define bMSG		(bLOGIN+1)
#define bNOTIFY		(bMSG+1)
#define bPID		(bNOTIFY+1)
#define bPING		(bPID+1)
#define bPORT		(bPING+1)
#define bSET		(bPORT+1)
#define bSHOW		(bSET+1)
#define bSHOWLIST	(bSHOW+1)
#define bSHOWORIG	(bSHOWLIST+1)
#define bSTATUS		(bSHOWORIG+1)
#define bTIME		(bSTATUS+1)
#define bUNLOCK		(bTIME+1)
#define bVERBOSE	(bUNLOCK+1)

struct GatedCommands {
	int token;
	char *key;
};

extern struct GatedCommands GatedCmds[];

#define gADD		1
#define gADDRESS	(gADD+1)
#define gDELETE		(gADDRESS+1)
#define gGUESS		(gDELETE+1)
#define gSEEN		(gGUESS+1)
#define gSTAT		(gSEEN+1)
#define gTOUCH		(gSTAT+1)
#define gUSER		(gTOUCH+1)
#define gWRITE		(gUSER+1)

struct WpdCommands {
	int token;
	char *key;
	int type;
	int length;
};

extern struct WpdCommands WpdCmds[];

#define wString		1
#define wNumber		2
#define wLevel		4
#define wCmd		8

#define wADDRESS	1
#define wBBS		(wADDRESS+1)
#define wCALL		(wBBS+1)
#define wCHANGED	(wCALL+1)
#define wCREATE		(wCHANGED+1)
#define wFNAME		(wCREATE+1)
#define wGUESS		(wFNAME+1)
#define wHLOC		(wGUESS+1)
#define wHOME		(wHLOC+1)
#define wKILL		(wHOME+1)
#define wLEVEL		(wKILL+1)
#define wQTH		(wLEVEL+1)
#define wSEARCH		(wQTH+1)
#define wSEEN		(wSEARCH+1)
#define wSHOW		(wSEEN+1)
#define wSTAT		(wSHOW+1)
#define wSYSOP		(wSTAT+1)
#define wUPDATE		(wSYSOP+1)
#define wUPLOAD		(wUPDATE+1)
#define wUSER		(wUPLOAD+1)
#define wWRITE		(wUSER+1)
#define wZIP		(wWRITE+1)


struct MsgdCommands {
	int token;
	char *key;
};

extern struct MsgdCommands MsgdCmds[];

#define mACTIVE		1
#define mAGE		(mACTIVE+1)
#define mBBS		(mAGE+1)
#define mCATCHUP	(mBBS+1)
#define mDEBUG		(mCATCHUP+1)
#define mDISP		(mDEBUG+1)
#define mEDIT		(mDISP+1)
#define mFLUSH		(mEDIT+1)
#define mFORWARD	(mFLUSH+1)
#define mGROUP		(mFORWARD+1)
#define mHOLD		(mGROUP+1)
#define mIMMUNE		(mHOLD+1)
#define mKILL		(mIMMUNE+1)
#define mKILLH		(mKILL+1)
#define mLIST		(mKILLH+1)
#define mMINE       (mLIST+1)
#define mNORMAL		(mMINE+1)
#define mOLD		(mNORMAL+1)
#define mPARSE		(mOLD+1)
#define mPENDING	(mPARSE+1)
#define mREAD		(mPENDING+1)
#define mREADH		(mREAD+1)
#define mREADRFC	(mREADH+1)
#define mREHASH		(mREADRFC+1)
#define mROUTE		(mREHASH+1)
#define mSEND		(mROUTE+1)
#define mSET		(mSEND+1)
#define mSINCE      (mSET+1)
#define mSTAT		(mSINCE+1)
#define mSYSOP		(mSTAT+1)
#define mUSER		(mSYSOP+1)
#define mWHO		(mUSER+1)
#define mWHY		(mWHO+1)

struct msg_dir_entry {
	struct msg_dir_entry *next, *last;
	long	number;
	long	size;
	long	flags;
	long	visible;

	char	list_text[256];
	char	bid[LenBID];
	char	sub[LenSUBJECT];
	char	passwd[LenPASSWD];

	struct msg_address {
		struct call_and_checksum
			name, at;
		char address[LenHLOC];
	} to, from;

	time_t	odate;
	time_t	kdate;
	time_t	cdate;
	time_t	edate;
	time_t	time2live;
	time_t	update_t;

	long	read_cnt;
	long	fwd_cnt;
	struct text_line *header;
	struct text_line *body;
	struct text_line *read_by;
};

#define	MsgActive		0x00000001
#define	MsgOld			0x00000002
#define	MsgHeld			0x00000004
#define	MsgKilled		0x00000008
#define MsgImmune		0x00000010
#define	MsgPending		0x00000020
#define	MsgStatusMask	0x0000001F

#define	MsgPersonal		0x00000040
#define	MsgBulletin		0x00000080
#define	MsgNTS			0x00000100
#define	MsgSecure		0x00000200
#define	MsgPassword		0x00000400
#define	MsgTypeMask		0x000003C0

#define	MsgLocal		0x00001000
#define	MsgCall			0x00002000
#define	MsgCategory		0x00004000
#define MsgToNts		0x00008000
#define MsgKindMask		0x00007000
#define MsgToMask		0x0000E000
#define	MsgRead			0x00010000
#define	MsgCheckedOut	0x00020000
#define MsgSentFromHere	0x00040000
#define MsgNoFwd		0x00080000

#define	MsgReadByMe		0x00200000

#define MsgWriteMask	0x002FFFFF

/* send flags */
#define MsgAtBbs		0x01000000
#define MsgAtDist		0x02000000
#define MsgAtMask		0x03000000
#define MsgHloc			0x10000000
#define MsgFrom			0x20000000
#define MsgBid			0x40000000

/* list flags */
#define	MsgMine			0x00100000
#define	MsgClub			0x00400000
#define	MsgSkip			0x00800000
#define	MsgAll			0x01000000
#define	MsgSentByMe		0x02000000
#define	MsgNew			0x04000000
#define	MsgNotRead		0x08000000
#define	MsgNotReadByMe	0x10000000

#define MsgReadByServer	0x80000000

/* test for message_directory_entry.flags */

#define IsMsgActive(m)      (m->flags & MsgActive)
#define IsMsgOld(m)     (m->flags & MsgOld)
#define IsMsgHeld(m)        (m->flags & MsgHeld)
#define IsMsgKilled(m)      (m->flags & MsgKilled)
#define IsMsgImmune(m)      (m->flags & MsgImmune)


#define IsMsgPersonal(m)    (m->flags & MsgPersonal)
#define IsMsgBulletin(m)    (m->flags & MsgBulletin)
#define IsMsgNTS(m)     (m->flags & MsgNTS)
#define IsMsgSecure(m)      (m->flags & MsgSecure)
#define IsMsgPassword(m)    (m->flags & MsgPassword)


#define IsMsgPending(m)     (m->flags & MsgPending)
#define IsMsgNoFwd(m)       (m->flags & MsgNoFwd)


#define IsMsgLocal(m)       (m->flags & MsgLocal)
#define IsMsgCall(m)        (m->flags & MsgCall)
#define IsMsgCategory(m)    (m->flags & MsgCategory)

#define IsMsgRead(m)        (m->flags & MsgRead)
#define IsMsgCheckedOut(m)  (m->flags & MsgCheckedOut)
#define IsMsgCheckedIn(m)   (!(m->flags & MsgCheckedOut))

#define IsMsgAND(m,f)       ((m->flags & f) == f)
#define IsMsgOR(m,f)        (m->flags & f)

#define IsMsgMine(m)        (m->flags & MsgMine)
#define IsMsgReadByMe(m)    (m->flags & MsgReadByMe)
#define IsMsgClub(m)        (m->flags & MsgClub)
#define IsMsgSkip(m)        (m->flags & MsgSkip)
#define IsMsgAll(m)         (m->flags & MsgAll)
#define IsMsgSentByMe(m)    (m->flags & MsgSentByMe)

#define IsMsgRelated(m)     (m->flags & (MsgSentByMe|MsgMine))
#define HasServerRead(m)    (m->flags & MsgReadByServer)
#define IsMsgFromHere(m)    (m->flags & MsgSentFromHere)

struct UserdCommands {
	int token;
	char *key;
	int type;
	int length;
	long def;
};

extern struct UserdCommands UserdCmds[];

	/* userd fields */
#define uToggle		1
#define uString		2
#define uNumber		4
#define uCmd		8
#define uList		0x10

#define uADDRESS	1
#define uASCEND		(uADDRESS+1)
#define uAPPROVED	(uASCEND+1)
#define uBASE		(uAPPROVED+1)
#define uBBS		(uBASE+1)
#define uCOMPUTER	(uBBS+1)
#define uCOUNT		(uCOMPUTER+1)
#define uEMAIL		(uCOUNT+1)
#define uEMAILALL	(uEMAIL+1)
#define uEXCLUDE	(uEMAILALL+1)
#define uFLUSH		(uEXCLUDE+1)
#define uFREQ		(uFLUSH+1)
#define uHDUPLEX	(uFREQ+1)
#define uHELP		(uHDUPLEX+1)
#define uIMMUNE		(uHELP+1)
#define uINCLUDE	(uIMMUNE+1)
#define uLINES		(uINCLUDE+1)
#define uLNAME		(uLINES+1)
#define uLOG		(uLNAME+1)
#define uMACRO		(uLOG+1)
#define uMESSAGE	(uMACRO+1)
#define uNEWLINE	(uMESSAGE+1)
#define uNONHAM		(uNEWLINE+1)
#define uNUMBER		(uNONHAM+1)
#define uPHONE		(uNUMBER+1)
#define uPORT		(uPHONE+1)
#define uREGEXP		(uPORT+1)
#define uRIG		(uREGEXP+1)
#define uSIGNATURE	(uRIG+1)
#define uSOFTWARE	(uSIGNATURE+1)
#define uSTATUS		(uSOFTWARE+1)
#define uSUFFIX		(uSTATUS+1)
#define uSYSOP		(uSUFFIX+1)
#define uTNC		(uSYSOP+1)
#define uUPPERCASE	(uTNC+1)
#define uVACATION	(uUPPERCASE+1)

	/* userd commands */
#define uAGE		(uVACATION+1)
#define uCALL		(uAGE+1)
#define uCLEAR		(uCALL+1)
#define uCREATE		(uCLEAR+1)
#define uKILL		(uCREATE+1)
#define uLOCATE		(uKILL+1)
#define uLOGIN		(uLOCATE+1)
#define uSEARCH		(uLOGIN+1)
#define uSET		(uSEARCH+1)
#define uSHOW		(uSET+1)
#define uUSER		(uSHOW+1)

	/* criteria fields */

#define cSEPARATOR	':'
#define cTO			'>'
#define cFROM		'<'
#define cAT			'@'
#define cBID		'$'
#define cSUBJECT	'&'
#define cCOMMAND	'!'
#define cFLAG		'|'
#define cAGE_OLDER	'+'
#define cAGE_NEWER	'-'
#define cHLOC		'.'

	/*  rfc822 fields */

#define rBID		1
#define rBORN		(rBID+1)
#define rCREATE		(rBORN+1)
#define rFROM		(rCREATE+1)
#define rHELDBY		(rFROM+1)
#define rHELDREL	(rHELDBY+1)
#define rHELDWHY	(rHELDREL+1)
#define rIMMUNE		(rHELDWHY+1)
#define rKILL		(rIMMUNE+1)
#define rLIVE		(rKILL+1)
#define rPASSWORD	(rLIVE+1)
#define rREADBY		(rPASSWORD+1)
#define rREADBYME	(rREADBY+1)
#define rREPLY		(rREADBYME+1)
#define rSUBJECT	(rREPLY+1)
#define rTO			(rSUBJECT+1)
#define rTYPE		(rTO+1)

extern char *rfc_text[];

struct active_bbss {
	struct active_bbss *next;
	int proc_num;
	int via;
	int mask;
	int chat_port;
	char call[10];
	char text[1024];
	long t0;
};

struct callbook_entry {
	char	suffix[4];
	char	callarea[2];
	char	prefix[3];
	char	lname[25];
	char	fname[12];
	char	mname[2];	
	char	addr[36];
	char	city[21];
	char	state[3];
	char	zip[6];
	char	birth[3];
	char	exp[6];
	char	class[2];
};

struct callbook_index {
	char key[8];
	char area;
	char suffix;
	uint8_t pad[2];
	uint8_t loc_xdr[4];
};

struct PortDefinition {
	struct PortDefinition *next;
	int secure, type;
	char *name;
	char alias[9];
	int show;
};

/*
 * Kenwood has line of mobile radios with built-in TNCs that can be
 * used in KISS mode. The TNCs in some of these radios, however, have 
 * a bug which will cause them to exit KISS mode if they see a string
 * matching the radio command "TC 0".
 *
 * A workaround for such radios is to ensure that no ASCII "C" character
 * makes it unescaped into the KISS stream. Setting this flag on a TNC's
 * ax25_params.flags member will cause TNCD to make such an adjustment.
 */
#define TNC_AX25_ESCAPE_ASCII_C 1

struct ax25_params {
		/* AX25 parameters
		 * t1 = This is how long to wait after a frame is sent before
		 *      sending a poll request for error recovery.
		 * t2 = The amount of time to wait after a frame is received for
		 *		another frame. If the timer expires before
		 *              another frame is received an ACK is sent.
		 * t3 = How often to ping the remote site if no other frame
		 *		has been received.
		 * maxframe = maximum number of frames to send in a row without
		 *				requiring an acknowlegement.
		 * paclen = maximum packet size in bytes. Typical setting would
		 *			be 110, should never go beyond 256.
		 * n2 = number of retries to allow.
		 * flags = Special KISS encoding flags, in ASCII hexadecimal:
		 *   1 = Escape ASCII 'C' on this port
		 * pthresh = theshold below which an unack'd packet will be
		 *   simply resent vs being queried.
		 */

	int t1, t2, t3;
	int maxframe, paclen, pthresh;
	int n2;
	int flags;
};

struct TncDefinition {
	struct TncDefinition *next;
	char *name;
	char *control_bind_addr;
	int   control_port;
	char *monitor_bind_addr;
	int   monitor_port;
	char *device;
	char *host;
	struct ax25_params ax25;
};
	
struct PhoneDefinition {
	struct PhoneDefinition *next;
	char *name;
	char *device;
	char *init_str;
};

int
	callbk(void),
	lookup_call_callbk(char *call, struct callbook_entry *cb);

void
	msgd_debug(int level),
	gen_callbk_body(struct text_line **tl),
	callbk_server(int num, char *mode),
	init_time(void),
	parse_options(int argc, char *argv[],  struct ConfigurationList *cl, char *me),
	show_configuration_rules(char *fn),
	show_reqd_configuration(struct ConfigurationList *cl, char *proc_name, char *fn),
	log_f(const char *name, const char *s, const char *p),
	log_clear(const char *name),
	msgd_close(void),
	logd(const char *string),                                  /* logd.c */
	logd_stamp(const char *whoami, const char *string),        /* logd.c */
	logd_close(void),                                          /* logd.c */
	wpd_close(void),						/* wpd.c */
	userd_close(void),						/* userd.c */
	gated_close(void);						/* gated.c */

int
	message_matches_criteria(char *match, struct msg_dir_entry *m, long timenow),
	msgd_open(void),
	msgd_fetch_multi(char *cmd, void (*callback)(char *s)),
	msgd_fetch_textline(char *cmd, struct text_line **tl),
	logd_open(const char *whoami),                             /* logd.c */
	parse_callsign(char *call),				/* common.c */
	get_daemon_version(char *id),
	wpd_open(void),							/* wpd.c */
	wpd_open_specific(char *hostname, int port),
	userd_open(void),						/* userd.c */
	userd_fetch_multi(char *cmd, void (*callback)(char *s)),	/* userd.c */
	wpd_fetch_multi(char *cmd, void (*callback)(char *s)),	/* wpd.c */
	gated_fetch_multi(char *cmd, void (*callback)(char *s)),	/* gated.c */
	gated_open(void);						/* gated.c */

char
    *daemon_version(char *name, char *call),
	*rfc822_find(int token, struct text_line *tl),
	*rfc822_xlate(int field),
	*msgd_read(void),
	*msgd_xlate(int token),
	*msgd_cmd(char *cmd),
	*msgd_cmd_num(char *cmd, int num),
	*msgd_cmd_string(char *cmd, char *s),
	*msgd_xlate(int token),
	*msgd_cmd_textline(char *cmd, struct text_line *tl),
	*wpd_fetch(char *cmd),					/* wpd.c */
	*wpd_read(void),						/* wpd.c */
	*userd_fetch(char *cmd),				/* userd.c */
	*userd_read(void),						/* userd.c */
	*gated_read(void),						/* gated.c */
	*gated_fetch(char *cmd);				/* gated.c */

int
        msg_ReadRfc(struct msg_dir_entry *m),
	msg_SendMessage(struct msg_dir_entry *m),
	msg_ReadBody(struct msg_dir_entry *m),
	msg_ReadBodyBy(struct msg_dir_entry *m, char *by),
	mid_chk(char *bid),
	bid_chk(char *bid),
	bid_delete(char *bid),
	bid_add(char *bid);

char
	*msg_SetActive(int number),
	*msg_SetImmune(int number),
	*msg_SetHeld(int number, struct text_line *reason),
	*msg_SysopMode(void),
	*msg_BbsMode(void),
	*msg_CatchUp(void),
	*msg_NormalMode(void),
	*msg_ListMineMode(void),
	*msg_ListSinceMode(long t),
	*msg_LoginUser(char *call);


struct text_line
	*msg_WhyHeld(int number);

int
#ifndef SABER
	error_log(char *fmt, ...);
#else
	error_log();
#endif

long msg_ListMode(void);
#if 0
	get_time(long *t),
#endif

time_t
	str2time_t(char *s),
	bbsd_get_time(void);

int
	rfc822_parse(struct msg_dir_entry *m, char *buf),
	port_indx(char *name),
	port_secure(char *name),
	port_type(char *name),
	port_show(char *name),
	tnc_port(char *name),
	tnc_connect(char *host, int port, char *dest, char *mycall),
	tnc_set_ax25(struct ax25_params *ax25),
	tnc_monitor_port(char *name),
	bbsd_socket(void),
	bbsd_msg(char *str),
	bbsd_get_configuration(struct ConfigurationList *cl),
	bbsd_get_variable_list(char *var, void (*callback)(char *s)),
	bbsd_open(char *host, int port, char *name, char *via),
	bbsd_pid(void),
	bbsd_ping(void),
	bbsd_version(void),
	bbsd_login(char *name, char *via),
	bbsd_cmd(char *cmd),
	bbsd_cmd_num(char *cmd, int num),
	bbsd_cmd_string(char *cmd, char *s),
	bbsd_fetch_multi(char *cmd, void (*callback)(char *s)),
	bbsd_fetch_textline(char *cmd, struct text_line **tl),
	bbsd_port(int number),
	bbsd_lock(char *via, char *reason),
	bbsd_unlock(char *via),
	bbsd_set_variable(char *var, char *val),
	bbsd_check(char *call, char *via),
	bbsd_chk_lock(char *via),
	bbsd_get_status(struct text_line **tl),
	bbsd_chat_request(void),
	bbsd_chat_cancel(void),
	bbsd_notify_on(void),
	bbsd_notify_off(void);

void
	bbsd_close(void);

char
	*build_sid(void),
	*port_name(int indx),
	*port_alias(char *name),
	*tnc_device(char *name),
	*tnc_host(char *name),
	*tnc_control_bind_addr(char *name),
	*tnc_monitor_bind_addr(char *name),
	*phone_device(char *name),
	*phone_init(char *name),
	*bbsd_read(void),
	*bbsd_prefix_msg(char *str),
	*bbsd_xlate(int token),
	*bbsd_fetch(char *cmd),
	*bbsd_get_variable(char *var),
	*bbsd_get_orig_variable(char *var);

struct ax25_params
	*tnc_ax25(char *name);

struct PortDefinition
	*port_find(char *name),
	*port_table(void);

struct TncDefinition *tnc_table(void);
struct PhoneDefinition *phone_table(void);

int parse_remote_addr(const char *addrspec, struct RemoteAddr *addr);
int print_remote_addr(const struct RemoteAddr *addr, char *str, size_t len);
int get_remote_addr(int sock, struct RemoteAddr *addr);
