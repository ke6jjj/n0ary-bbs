#!/bin/sh

#
# BBS home directory.
#
BBS_HOME_DIR=XXBBS_DIRXX

#
# This is the default location from which to fetch the current FCC ULS
# Amateur Radio data.
#
DEFAULT_ULS_URL=https://data.fcc.gov/download/pub/uls/complete/l_amat.zip

#
# The same, but for Canadian amateurs, from ISEDC (Innovation, Science and
# Economic Development Canada).
#
DEFAULT_ISEDC_URL=https://apc-cap.ic.gc.ca/datafiles/amateur_delim.zip

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
# Add a directory to be removed on abnormal script exit.
#
add_cleanup() {
	cleanup_dirs="${cleanup_dirs} $1"
}

#
# Display an error message, clean up temporary directories, and exit
# with the provided code.
#
error() {
	for dir in ${cleanup_dirs}; do
		rm -rf "${dir}"
	done
	echo "$2" 1>&2
	exit $1
}

if [ $# -gt 1 ]; then
	uls_url="$1"
else
	uls_url="$DEFAULT_ULS_URL"
fi
if [ $# -gt 2 ]; then
	isedc_url="$2"
else
	isedc_url="$DEFAULT_ISEDC_URL"
fi

cleanup_dirs=

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

# Ask BBSD for the BBS callbook path
if ! get_bbs_variable "Bbs_Callbk_Path"; then
	error 1 "Can't get callbook directory (Bbs_Callbk_Path variable)"
fi

# The callbook path is relative to the bbs home directory. Expand it.
bbs_cbdir=${BBS_HOME_DIR}/${result}

# Make a temporary extraction directory
if ! tmpdir=`mktemp -d -t callbk-uls-XXXXXXXX`; then
	error 1 "Failed to create temporary extraction dir."
fi

add_cleanup ${tmpdir}

# Create a new temporary callbook directory in the BBS home directory.
# We will move this temporary directory into place once all processing is done.
if ! new_cbdir=`mktemp -d ${bbs_cbdir}.new.XXXXXXX`; then
	error 1 "Failed to create new callbook directory"
fi

add_cleanup ${new_cbdir}

# Fetch the current callbook data from the FCC.
echo "Fetching current FCC ULS Amateur Radio database."
zip_file=${tmpdir}/uls.zip
if ! curl -k -s -o ${zip_file} ${uls_url}; then
	error 1 "ULS database fetch failed from ${uls_url}"
fi

# Fetch the current Canadian callbook data from ISEDC.
echo "Fetching current ISEDC (CA) Amateur Radio database."
ca_zip_file=${tmpdir}/isedc.zip
if ! curl -k -s -o ${ca_zip_file} ${isedc_url}; then
	error 1 "ISEDC database fetch failed from ${isedc_url}"
fi

# Unzip the data
echo "Unzipping data."
if ! unzip -q -d ${tmpdir} ${zip_file} AM.dat HD.dat EN.dat; then
	error 1 "Unzip failed"
fi
if ! unzip -q -d ${tmpdir} ${ca_zip_file} amateur_delim.txt; then
	error 1 "CA Unzip failed"
fi

# Process the records
echo "Processing records."
makecb=${BBS_HOME_DIR}/bin/makecb_uls
if ! ${makecb} \
	${new_cbdir} \
	${tmpdir}/AM.dat \
	${tmpdir}/HD.dat \
	${tmpdir}/EN.dat \
	${tmpdir}/amateur_delim.txt 1>/dev/null; then
	error 1 "Callbook processing failed".
fi

# Sort all the generated indicies
sort_cb=${BBS_HOME_DIR}/bin/sort_cb
for index in city firstname lastname zip; do
	${sort_cb} ${new_cbdir}/${index}.indx
done

# Make the new directory world readable (and group writable)
chmod -R 755 ${new_cbdir}

# Move old directory aside
mv ${bbs_cbdir} ${bbs_cbdir}.old

# Move new directory into place
mv ${new_cbdir} ${bbs_cbdir}

rm -rf ${bbs_cbdir}.old
rm -rf ${tmpdir}

echo "Done."
