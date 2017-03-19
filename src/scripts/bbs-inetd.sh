#!/bin/sh
BBS_DIR=XXBBS_DIRXX
BBS_BIN=${BBS_DIR}/bin/b_bbs

echo ""
echo -n "login: "
read username
if [ "$username" == bbs ]; then
  while :; do
    echo -n "Enter your callsign (first name if non-ham): "
    read callsign
    filt_callsign=$( echo "$callsign" | sed -e 's,[^A-Za-z0-9]*,,g' )
    if [ "$filt_callsign" != "$callsign" ]; then
      echo ""
      echo "Error in callsign. Only letters and numbers are allowed"
      echo "please try again."
      continue
    fi
    if [ "${filt_callsign}" == "" ]; then
      continue
    fi
    echo "exec ${BBS_BIN} -t 1 -v TCP ${filt_callsign} 1<&0 2<&0"
  done
fi
exit 1
