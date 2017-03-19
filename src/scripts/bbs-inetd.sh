#!/bin/sh
BBS_DIR=XXBBS_DIRXX
BBS_BIN=${BBS_DIR}/bin/b_bbs

#
# Issue a login prompt and read the username.
#
echo ""
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
      echo ""
      echo "Error in callsign. Only letters and numbers are allowed"
      echo "please try again."
      continue
    fi
    if [ "${filt_callsign}" == "" ]; then
      continue
    fi

    #
    # Run the BBS!
    #
    exec ${BBS_BIN} -t 1 -v TCP "${filt_callsign}" 1<&0 2<&0
  done
fi
exit 1
