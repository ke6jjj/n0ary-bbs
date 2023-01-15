#!/bin/sh
#
# Initialize a new BBS from scratch, with safety checks so as to not destroy
# any existing installation.
#
get_variable() {
	_bbs_conf_path="$1"
	_varname="$2"

	set -- $( grep "^${_varname}" "${_bbs_conf_path}" ) || return 1
	echo $2
}

error() {
	echo "$1" 1>&2
	exit 1
}

get() {
	_varname="$1"

	_value=$( get_variable "$CONFIG_PATH" "$_varname" ) || \
		error "Can't find variable $_varname in '$CONFIG_PATH'."

	eval "${_varname}=${_value}" || error "Horrible eval!?"
}

directory() {
	_dirname="$1"
	_fulldir="${BBS_DIR}/$_dirname"

	mkdir -p "$_dirname" || error "Can't create directory $_dirname"
}

full_path() {
	_path="$1"
	echo "$BBS_DIR/$_path"
}

file() {
	_path="$1"
	_contents="$2"
	_contents2="$3"

	_fullpath=$( full_path "$_path" )
	_dirname=$( dirname "$_fullpath" )
	
	directory "$_dirname"
	[ -e "$_fullpath" ] && error "File $_fullpath aready exists...aborting."
	touch "$_fullpath" || error "Can't create file $_path."

	if [ -n "$_contents" ]; then
		echo "$_contents" > "$_fullpath" || \
			error "Can't write to $_fullpath"
		if [ -n "$_contents2" ]; then
			echo "$_contents2" >> "$_fullpath" || \
				error "Can't write to $_fullpath"
		fi
	fi
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

get BBS_CALL
get BBS_HLOC
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
file "$BIDD_FILE" "#"
file "$GATED_FILE" "#"
file "$LOGD_FILE"
directory "$LOGD_DIR"
directory "$MSGD_BODY_PATH"
directory "$MSGD_FWD_DIR"
directory "$USERD_ACC_PATH"
file "$WPD_USER_FILE" "# v1 hello" "+$BBS_CALL 0 0 0 0 0 $BBS_CALL ? ? ?"
file "$WPD_BBS_FILE" "# v1 hello" "+$BBS_CALL 0 $BBS_HLOC"
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
