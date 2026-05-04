#!/bin/sh

#
# BBS home directory.
#
BBS_HOME_DIR=XXBBS_DIRXX

#
# Fetch a bbs variable from BBSD
#
get_bbs_variable() {
	result=`printf "SHOW $1\r\nEXIT\r\n" | \
		nc ${bbsd_host} ${bbsd_port} | (
		read header;
		read something;
		read result;
		echo ${result}
	)`
	if [ "${result}" == "NO, Variable not found" ]; then
		return 1
	fi
	if [ "${result}" == "" ]; then
		return 1
	fi
	return 0
}

#
# Obtain a particular TNCD's process ID from BBSD
#
get_tncd_pid() {
	upc_tnc=$( echo $1 | tr a-z A-Z )
	result=`printf "STATUS PROCS\r\nEXIT\r\n" | \
		nc ${bbsd_host} ${bbsd_port} | (
		read header;
		read something;
		while :; do
			read pid_line;
			set -- $pid_line;
			# num procname type ctlport pid
			num=$1;
			procname=$2;
			pid=$5;
			if [ "${num}" == "." ]; then
				break;
			fi
			if [ "${procname}" == "$upc_tnc" ]; then
				if [ "$pid" -ne -1 ]; then
					echo $pid
				fi
				break;
			fi
		done
	)`
	if [ "${result}" == "" ]; then
		return 1
	fi
	return 0
}

#
# Display an error message and exit with the provided code.
#
error() {
	echo "$2" 1>&2
	exit $1
}

if [ $# -lt 1 ]; then
	error 1 "usage: $0 <tnc>"
fi
tnc=$( echo $1 | tr A-Z a-z )

#
# Read import variables from BBS config file
#
bbs_config=${BBS_HOME_DIR}/etc/Config
if [ ! -f ${bbs_config} ]; then
	error 1 "Can't read BBS config file"
fi
if ! bbsd_host_line=`grep BBS_HOST ${bbs_config}`; then
	error 1 "Can't determine BBS host"
fi
if ! bbsd_port_line=`grep BBSD_PORT ${bbs_config}`; then
	error 1 "Can't determine BBSD port"
fi

bbsd_host=`echo ${bbsd_host_line} | awk '{print $2;}'`
bbsd_port=`echo ${bbsd_port_line} | awk '{print $2;}'`

# Ask BBSD for the pid of the daemon controlling the given TNC.
if ! get_tncd_pid "$tnc"; then
	error 1 "Can't get PID of tnc daemon for $tnc"
fi
tncd_pid=${result}

# Ask BBSD where this TNCD logs its pcap files.
var=${tnc}_PCAP_DUMP_PATH
if ! get_bbs_variable $var; then
	error 1 "PCAP logging for $tnc doesn't appear to be enabled."
fi
active_pcap_file="${result}"

# Get directory part of PCAP log path
active_pcap_dir=$( dirname $active_pcap_file ) || \
	error 1 "Can't figure out PCAP logging path."

now=$( date +%s )
yesterday=$(( $now - 86400 ))
yesterday_iso=$( date -r $yesterday +%F )

archive_target="${active_pcap_dir}/${yesterday_iso}-${tnc}.pcap"

mv -i ${active_pcap_file} ${archive_target} || \
	error 1 "Unable to move ${active_pcap_file} to ${archive_target}"

kill -1 ${tncd_pid} || \
	error 1 "Unable to send SIGHUP to $tnc TNC daemon."

exit 0
