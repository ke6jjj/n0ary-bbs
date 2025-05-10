# n0ary-bbs

A FreeBSD port of the venerable N0ARY packet radio BBS, which was originally written for SunOS.

# Prerequisites

To compile N0ARY in its current form you will need GNU Make (gmake) version
3.11 or later.

# Configuration 

Before compiling the BBS you will need to edit `src/include/site_config.mk`
to specify two important options: the target operating system and the BBS
install directory.

## Operating System

You must edit `src/include/site_config.mk` to tell the build system which
operating system you are running. There are two choices: FreeBSD or SunOS.


1. FreeBSD

    To compile for FreeBSD, make your site_config.mk read:

        FREEBSD=1

2. SunOS

    To compile for SunOS it is first assumed that you will be
    _cross-compiling_ for SunOS, because there is no way you'd be able
    to get modern GMake to run on SunOS. (If you do, let me know!).

    To cross compile for SunOS, make your site_config.mk first read:

        SUNOS=1

    *Further cross-compilation instructions to be written*

## BBS Installation Directory

The BBS requires its own dedicated directory on the host filesystem and to
correctly install it, the final "make install" rules need to know where this
directory is. To set it up, edit `src/include/site_config.mk` and set the
`BBS_DIR` variable:

    BBS_DIR=<bbs-home-directory>

One example would be `/usr/home/bbs`

# Compilation

To compile the BBS:

    cd src/
    gmake

# Installation

To install the BBS and get it running you'll need to take several steps,
each of which is detailed below. As a general outline, though, you'll need to:

1. Install the BBS binaries and scripts.
1. Establish a UNIX user for the BBS to run as.
1. Edit the BBS "Config" file and ensure that "BBS_DIR" is correct.
1. Run the bbs "bootstrap" script to set up the BBS for the first time.
1. If you'll be using TNCs, set up TNC device permissions and make them stick.
1. Make edits to the system to ensure the BBS starts up at boot.

Let's describe each of these steps in detail.

## Make install

After compiling all of the BBS you are ready to install the binaries and
other utilities. Do so by running

    cd src/
    gmake install

## BBS user ID

Next you'll need to ensure your system has a non-privileged user for the
BBS processes to run under. It's best if you use `bbs`.

## BBS Configuration

Next you'll need to get a basic Config file for the BBS set up.

To begin, you should probably copy `<bbs-dir>/etc/Config.sample` to `<bbs-dir>/etc/Config` and then edit it.

There are many comments inside the sample configuration file for setting things
up, but at this stage the most important thing to ensure is that the "BBS_DIR"
setting is correct.

## BBS Bootstrapping

After you have the Config file in place and ensured that "BBS_DIR" is set
correctly within it, you're ready to run the "bootstrap" script to make
sure that the BBS's various sub-directories and database files are initialized.

1. Ensure that the BBS_DIR directory is owned by the `bbs` user and its
   permissions are set up so that the `bbs` user can create sub-directories
   under it. (`-rwxr-xr-x`).

2. Become the BBS user (`sudo su -m bbs`).

3. Run the bootstrap script, pointing it at the BBS config file:

   `/usr/local/libexec/bbs-init.sh <bbs-dir>/etc/Config`

## Hardware Access

If you intend to have the BBS use serially-connected TNCs (radio modems) then
you need to ensure that the bbs "user" has the ability to interact with the
serial port devices you've configured in the configuration file. This will
require your intervention to set up because most UNIX operating systems do not,
by default, allow non-root users to access serial ports at will.

(Note: this section does not apply if you have configured the BBS to connect
to a network terminal server to access the TNCs. That access is controlled
via the networking stack and/or system firewall).

When running under FreeBSD you can ensure that the BBS user is granted serial
port access by modifying the configuration file(s) that control device setup
at boot time. There are two ways to control device access under FreeBSD:

1. Through the "devfs" rules, which are loaded at boot time and are
   interpreted by the FreeBSD kernel (see devfs.rules(5)), and

2. Through the "devd.conf" configuration file, which controls the "devd"
   device daemon (devd(8)).

It's outside the scope of this document to fully describe these two methods,
but as an example, here's a sample `devfs.rules` file for method #1. It
ensures that the devices `/dev/cuaU0`, `/dev/cuaU1`, `/dev/cuaU2` and
`/dev/cuaU3` are all "owned" by the `bbs` user and have appropriate read/write
permissions before the BBS starts up:

```
[localrules=100]
# Ensure that the serial ports for tnc0, 1, 2 and 3 are owned by
# the BBS.
add path "cuaU[0-3]" mode 0660 user bbs group dialer
```

## Startup

The last step in setting up the BBS is to ensure that it starts up at
system boot (or, alternatively, starts up manually when you ask for it).

The BBS comes with an etc/rc.d style BSD startup script which will
automatically start up the BBS when the system starts up, provided you setup
`/etc/rc.conf` to do so. To simply enable BBS startup set:

    n0ary_bbs_enable=YES

Some additional variables you can set are:

* `n0ary_bbs_dir=`

    `n0ary_bbs_dir` tells the startup scripts where the BBS home directory
    is, but it generally needn't be set unless you move the BBS to a different
    directory than the one you set in `site_config.mk` at build time.

* `n0ary_bbs__user=`

    NOTE: TWO UNDERSCORES BETWEEN `bbs` and `user`. Do not accidentally only
    use one or the system will fail to start up.

    The BBS is a federation of several daemons that run in the background. For
    system security, these daemons should run as an unpriviledged user. The
    startup script will arrange for the daemons to run under the user id of
    the username you provide here. The default is `bbs`, which you should have
    set up previously during the installation step.

## Manual Startup

After you've set things up above, you can get the BBS started by running
the startup script:

  `sudo /usr/local/etc/rc.d/n0ary_bbs start`

## Logging In Locally

After you've started up the BBS, you'll probably want to test what it's
like to "log in" to it. Logging in to the BBS locally is merely a matter
of spawning the "bbs" execuable with the proper flags to interact with
the user via stdout/stdin and to reflect that you're logging in via the
"CONSOLE" device.

  `sudo su -m bbs -c '<BBS_DIR>/bin/b_bbs -u -v CONSOLE <SYSOP-CALLSIGN>`

# Periodic Tasks

Congratulations! The BBS should now be running. Your next steps are to
begin "administrating" the BBS by ensuring that various tasks are scheduled
to run periodically by your system.

After the BBS has been installed and is running, you should begin setting
up several periodic tasks to run through "cron" -- the UNIX task scheduling
daemon. These tasks include outbound message forwarding and updating the
US & Canada callbook database.

## Message forwarding

Once your BBS is configured and running, you will likely need to begin
forwarding messages and traffic to other BBSes. The international BBS routing
system is too complicated to describe here in detail, but the basics are
to setup `<bbs-dir>/etc/Systems` and `<bbs-dir>/etc/Route` to describe the
nearest neighbors to your BBS and how to reach them.

After you've set up the systems and routes you must then set up the message
forwarding cycles by using `cron` to launch the BBS in an appropriate
forwarding mode and on the schedules you desire. Here's an ancient, unchecked
example from the original SunOS setup instructions. This example assumes
that the BBS home directory is `/nbbs`.

```
    0,20,40 * * * * /nbbs/bin/b_bbs -t6 -vTNC1
    20 * * * * /nbbs/bin/b_bbs -t6 -vTNC0
    3,13,23,33,43,53 * * * * /nbbs/bin/b_bbs -t6 -vSMTP
    7,17,27,37,47,57 * * * * /nbbs/bin/b_bbs -t6 -vTCP
    1,11,21,31,41,51 * * * * /nbbs/bin/b_process
```

## Callbook Database

The BBS has the ability to use a local callsign database to greet new users by
name and provide callsign search and lookup services. To use these features you
must first build the callsign database, and to keep it up to date, you should
periodically re-build it from up-to-date data.

Both of these tasks are wrapped into a single shell script which the
installation process should have deposited in

`/usr/local/libexec/callbook-update-uls`

This script can be invoked manually (through a terminal session) or through an
automated system (like cron) to cleanly, and safely update (or create) the
callbook database, even while there are users connected and other active
sessions running.

IMPORTANT: The script should be run as the "bbs" user you set up for the BBS.
If you run it as a different user (or as root), you may find that the BBS is
unable to use the database at runtime, or that it is impossible to update
later (due to permission errors).

## Update details

The update script will download the most recent US FCC _AND_ Canadian ISEDC
Amateur Radio licensing data, uncompress it, compile and index it in the
format used by the BBS. It will then atomically move the old database aside
while installing the new database in its place (using a directory rename).
Finally, upon success, it will delete the old database.

An example crontab entry which completes this task once a week looks like this:

```
# Update BBS callbook database from ULS every Thursday
12 1 * * 4	/usr/local/libexec/callbook-update-uls
```

(Again, this works best when installing this entry in the _bbs_ user's
crontab).

## Dependence on bbsd

The script is coded to be quite robust in obeying your particular local setup.
This means that it does its best to discover where the BBS is installed and
even further, whether you have customized the location of the callsign database
via the `Bbs_Callbk_Path` variable. For this reason, the BBS's `bbsd` process
must be running when the update is attempted. If `bbsd` is not running, the
script will complain and refuse to run.
