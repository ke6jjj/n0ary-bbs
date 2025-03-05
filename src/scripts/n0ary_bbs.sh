#!/bin/sh
#
# PROVIDE: n0ary_bbs
# REQUIRE: LOGIN
# KEYWORD: shutdown
#

. /etc/rc.subr

name=n0ary_bbs
rcvar=`set_rcvar`

load_rc_config ${name}

: ${n0ary_bbs_enable:="NO"}
: ${n0ary_bbs_dir="XXBBS_DIRXX"}
: ${n0ary_bbs_bin="${n0ary_bbs_dir}/bin/b_bbsd"}
: ${n0ary_bbs_conf="${n0ary_bbs_dir}/etc/Config"}
: ${n0ary_bbs__user=bbs}
: ${n0ary_bbs_pidfile="/var/run/n0ary_bbs.pid"}

required_files="${n0ary_bbs_conf}"
pidfile="${n0ary_bbs_pidfile}"
command="/usr/sbin/daemon"
command_args="-c -P ${pidfile} -R 30 -S -u ${n0ary_bbs__user}"

if [ "${n0ary_bbs_debug}" == "YES" ]; then
	# Remain in foreground, add verbose logging
	debug_flags="0x101"
else
	# Remain in foreground
        debug_flags="0x100"
fi

process_args="-d ${debug_flags} ${n0ary_bbs_conf}"

command_args="${command_args} ${n0ary_bbs_bin} ${process_args}"

# Customized kill command to send signal to entire process group.
_run_rc_killcmd()
{
        local _cmd

        _cmd="kill -$1 -- -$rc_pid"
        if [ -n "$_user" ]; then
                _cmd="su -m ${_user} -c 'sh -c \"${_cmd}\"'"
        fi
        echo "$_cmd"
}

run_rc_command "$1"
