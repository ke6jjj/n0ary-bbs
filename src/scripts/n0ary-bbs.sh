#!/bin/sh
#
# PROVIDE: n0ary_bbs
# REQUIRE: LOGIN
# KEYWORD: shutdown
#
. /etc/rc.subr

load_rc_config n0ary_bbs

: ${n0ary_bbs_enable:="NO"}

n0ary_bbs_dir=${n0ary_bbs_dir:="XXBBS_DIRXX"}
n0ary_bbs_bin="${n0ary_bbs_dir}/bin/b_bbsd"
n0ary_bbs_conf="${n0ary_bbs_dir}/etc/config"
n0ary_bbs_user="${n0ary_bbs_user:=bbs}
PIDFILE=${n0ary_bbs_pidfile:="/var/run/n0ary_bbs.pid"}

case "${n0ary_bbs_enable}" in
  YES|Yes|yes)
    n0ary_bbs_enable=1
    ;;
  NO|No|no|*)
    n0ary_bbs_enable=0
    ;;
esac

case "$1" in
  "start")
    if [ ${n0ary_bbs_enable} == 0 ]; then
      exit 0
    fi
    echo "Starting N0ARY BBS..."
    su -m ${n0ary_bbs_user} -c "${n0ary_bbs_bin} ${n0ary_bbs_conf}"
    echo $! > ${PIDFILE}
    echo "done"
  ;;

  "stop")
    echo "Stopping N0ARY BBS..."
    if [ -f ${PIDFILE} ] ; then
      kill `cat ${PIDFILE}`
      rm ${PIDFILE}
      echo "done"
    else
      echo "not running?"
    fi
  ;;

  "restart")
    echo "Restarting N0ARY BBS..."
    $0 stop
    sleep 2
    $0 start
  ;;

  *)
    echo "$0 [start|stop|restart]"
  ;;

esac
