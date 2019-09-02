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

To install the BBS you'll need to run the `make install` rule, you'll need
to set up a user ID for the bbs to run under, and finally you'll need to
edit the BBS configuration file, `<bbs-dir>/etc/Config`

## Make install

    cd src/
    gmake install

## BBS user ID

Create a user for the BBS to run under. Perhaps use `bbs`.

## BBS Configuration

The BBS has many options to configure. To begin, you should probably copy
`<bbs-dir>/etc/Config.sample` to `<bbs-dir>/etc/Config` and then edit it.
There are many comments inside the sample configuration file for setting things
up.

# Startup

The BBS comes with an etc/rc.d style BSD startup script which will
automatically start up the BBS when the system starts up, provided you setup
`/etc/rc.conf` to do so. To simply enable BBS startup set:

    n0ary_bbs_enable=YES

Some additional variables you can set are:

* `n0ary_bbs_dir=`

    `n0ary_bbs_dir` tells the startup scripts where the BBS home directory
    is, but it generally needn't be set unless you move the BBS to a different
    directory than the one you set in `site_config.mk` at build time.

* `n0ary_bbs_user=`

    The BBS is a federation of several daemons that run in the background. For
    system security, these daemons should run as an unpriviledged user. The
    startup script will arrange for the daemons to run under the user id of
    the username you provide here. The default is `bbs`, which you should have
    set up previously during the installation step.

# Forwarding and other tasks

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

    0,20,40 * * * * /nbbs/bin/b_bbs -t6 -vTNC1
    20 * * * * /nbbs/bin/b_bbs -t6 -vTNC0
    3,13,23,33,43,53 * * * * /nbbs/bin/b_bbs -t6 -vSMTP
    7,17,27,37,47,57 * * * * /nbbs/bin/b_bbs -t6 -vTCP
    1,11,21,31,41,51 * * * * /nbbs/bin/b_process

