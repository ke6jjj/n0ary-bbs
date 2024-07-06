#!/bin/sh
BBS_DIR=XXBBS_DIRXX
BBS_BIN=${BBS_DIR}/bin/b_bbs
GETPEERNAME=${BBS_DIR}/bin/getpeername

# On SIGPIPE, just exit.
trap 'exit 0' PIPE

#
# Get remote address.
#
if ! remote_addr=$( ${GETPEERNAME} ); then
  echo "Invalid peername."
fi
set -- ${remote_addr}
af=$1
host=$2
port=$3

case ${af} in
inet)
  remote=tcpip:${host}:${port}
  ;;
inet6)
  remote=tcpipv6:${host}:${port}
  ;;
*)
  remote=unknown:
esac

#
# Issue a login prompt and read the username.
#
printf "\r\n"
echo -n "login: "
if ! read username; then
  exit 0
fi

#
# This script is run directly with the remote socket on stdin. This
# means that the read command will pick up the traditional carriage
# return character that is sent with every newline. We need to remove
# it from the username received.
#
username=$( echo "$username" | sed -e 's,[^A-Za-z0-9]*,,g' )

if [ "$username" == bbs ]; then
  #
  # User wishes to log in to BBS. Get a callsign.
  #
  while :; do
    echo -n "Enter your callsign (first name if non-ham): "
    if ! read callsign; then
      exit 1
    fi

    #
    # As with the telnet username, the callsign must be filtered for
    # trailing carriage returns.
    #
    nospc_callsign=$( echo "$callsign" | sed -e 's,[[:space:]]*,,g' )

    #
    # Additionally, we need to keep the callsign to alphanumeric characters
    # only.
    #
    filt_callsign=$( echo "$nospc_callsign" | sed -e 's,[^A-Za-z0-9]*,,g' )

    #
    # Warn the user if the callsign had bad characters in it.
    #
    if [ "$filt_callsign" != "$nospc_callsign" ]; then
      printf "\r\n"
      printf "Error in callsign. Only letters and numbers are allowed\r\n"
      printf "please try again.\r\n"
      continue
    fi
    if [ "${filt_callsign}" == "" ]; then
      continue
    fi

    #
    # Issue a newline so that the BBS SID starts out on a fresh line.
    #
    printf "\r\n"

    #
    # Run the BBS!
    #
    exec ${BBS_BIN} -l -t 1 -v TCP -a ${remote} "${filt_callsign}" 1<&0 2<&0
  done
else
  # Honeypot trap
  echo -n "Password: "
  prompt=">"
  ignore=1
  while read line; do
    echo $( date '+%Y-%m-%dT%H:%M:%S' ) $remote "${line}" >> /var/log/honeypot
    set -- ${line}
    cmd=$1
    if [ ${ignore} -eq 1 ]; then
      ignore=0
    else
      if [ "${cmd}" == "sh" ]; then
        echo ""
        echo ""
        echo "BusyBox v1.00 (2010.06.21-09:23+0000) Built-in shell (msh)"
        echo "Enter 'help' for a list of built-in commands."
        prompt="#"
      elif [ "${cmd}" == "/bin/busybox" ]; then
        if [ $# -gt 1 ]; then
          cmd=$2
          if [ "${cmd}" == "wget" ]; then
            if [ $# -gt 2 ]; then
              echo "You wish."
            else
              echo "usage: wget [-c|--continue] [-s|--spider] [-q|--quiet] [-O|--output-document file]"
            fi
          else
            echo "${2}: applet not found"
          fi
        else
          echo "BusyBox v1.00 (2010.06.21-09:23+0000) multi-call binary"
          echo ""
          echo "Usage: busybox [function] [arguments]..."
        fi
      else
        echo "${cmd}: not found"
      fi
    fi

    printf "\r\n${prompt} "
  done
fi
exit 1
