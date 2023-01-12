#!/bin/sh
#
# Initialize a new BBS from scratch, with safety checks so as to not destroy
# any existing installation.
#
get_variable() {
	_bbs_conf_path="$1"
	_varname="$2"

	grep "${_bbs_conf_path}" "^${_varname}"
}

error() {
	echo "$1" 1>&2
	exit 1
}

get() {
	_varname="$1"

	_value=$( get_variable "$CONFIG_PATH" "$_varname" ) || \
		error "Can't find variable $_varname in '$CONFIG_PATH'."

	eval "${_varname}='${_value}'" || error "Horrible eval!?"
}

directory() {
	_dirname="$1"
	_fulldir="${BBS_DIR}/$_dirname"

	mkdir -p "$_dirname" || error "Can't create directory $_dirname"
}

file() {
	_path="$1"

	_dirname=$( dirname "$_path" )
	
	directory "$_dirname"
	[ -e "$_path" ] && error "File $_path aready exists...aborting."
	touch "$_path" || error "Can't create file $_path."
}

maybe_get() {
	_varname="$1"

	_value=$( get_variable "$CONFIG_PATH" "$_varname" ) || return 0

	eval "${_varname}='${_value}'" || error "Horrible eval!?"
}

if [ $# != 1 ]; then
	error "usage: $0 <path-to-bbs-config-file>"
fi

CONFIG_PATH="$1"; shift

get BBS_DIR
get Bbs_Event_Path
get Bbs_Event_Dir
get BIDD_FILE
get GATED_FILE
get LOGD_FILE
get LOGD_DIR
get MSGD_BODY_PATH
get MSGD_FWD_DIR
get USERD_ACC_PATH
get WPD_USER_FILE
get WPD_BBS_FILE
get WPD_DUMP_FILE
get Bbs_History_File
get Bbs_History_Path
get Bbs_WxLog_File
get Bbs_WxData_File
get Bbs_WxOutdoor_File
get Bbs_WxOutdoorYest_File
get Bbs_WxIndoor_File
get Bbs_WxIndoorYest_File
get Bbs_Sign_Path
get Bbs_Callbk_Path
get Bbs_Info_Path
get Bbs_Distrib_Path
get Bbs_Vacation_Path
get Bbs_FileSys_Path
get Bbs_Log_Path
get GATE_SPOOL_DIR
# Optional features
maybe_get MSGD_ARCHIVE_PATH	message/Archive

# d = ensure directory, e = ensure empty file, t = example file
directory "${Bbs_Event_Path}/Body"
file "$Bbs_Event_Dir"
file "$BIDD_FILE"
file "$GATED_FILE"
file "$LOGD_FILE"
directory "$LOGD_DIR"
directory "$MSGD_BODY_PATH"
directory "$MSGD_FWD_DIR"
directory "$MSGD_ARCHIVE_PATH"
directory "$USERD_ACC_PATH"
file "$WPD_USER_FILE"
file "$WPD_BBS_FILE"
file "$WPD_DUMP_FILE"
file "$Bbs_History_File"
directory "$Bbs_History_Path"
file "$Bbs_WxLog_File"
file "$Bbs_WxData_File"
file "$Bbs_WxOutdoor_File"
file "$Bbs_WxOutdoorYest_File"
file "$Bbs_WxIndoor_File"
file "$Bbs_WxIndoorYest_File"
directory "$Bbs_Sign_Path"
directory "$Bbs_Callbk_Path"
file "$Bbs_Info_Path"
directory "$Bbs_Distrib_Path"
directory "$Bbs_Vacation_Path"
directory "$Bbs_FileSys_Path"
directory "$Bbs_Log_Path"
directory "$GATE_SPOOL_DIR"
if [ ! -z "${MSGD_ARCHIVE_PATH}" ]; then
	directory "${MSGD_ARCHIVE_PATH}"
fi

echo "Initialization complete."
