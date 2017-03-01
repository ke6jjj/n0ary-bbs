#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>

#include "slab.h"
#include "am_database.h"
#include "am_record.h"
#include "call_id.h"
#include "bbslib.h"
#include "callbk.h" /* From BBS */

struct callbook_out_context {
	FILE *fp_city;
	FILE *fp_first;
	FILE *fp_last;
	FILE *fp_zip;
	const char *outdir;
};

static void usage(const char *prog);
static void setup_out_context(const char *outdir,
	struct callbook_out_context *cbf);
static void close_out_context(struct callbook_out_context *cbf);
static void load_AMDatabase_from_AM_file(struct AMDatabase *AMDb,
	slab_t AMSlab, const char *path);
static void load_AMDatabase_from_HD_file(struct AMDatabase *AMDb,
	slab_t AMSlab, const char *path);
static void process_and_dump_EN_file(struct callbook_out_context *cbf,
	struct AMDatabase *AMDb, const char *ENpath);
static void test(struct AMDatabase *AMDb);
static void warn_skip(const char *prefix, size_t lineno, const char *reason);
static void warn_skip_ex(const char *prefix, size_t lineno, const char *reason,
                        const char *sample);
static void add_call(struct callbook_out_context *,
	uint32_t callid,
	const char *lname,
	const char *lname_suffix,
	const char *fname,
	const char *mname,
	const char *addr,
	const char *city,
	const char *state,
	const char *zip,
	const char *birth,
	uint16_t expire_year,
	uint8_t expire_month,
	uint8_t expire_day,
	char op_class
);
static int strncpy_upr(char *dst, const char *src, size_t max);
static uint16_t get_doy(unsigned int year, uint8_t month, uint8_t day);

int
main(int argc, char *argv[])
{
	slab_t AMSlab;
	struct AMDatabase AMDb;
	struct callbook_out_context cbf;

	if (argc < 5) {
		usage(argv[0]);
		exit(1);
	}

	if (slab_init(sizeof(struct ULS_AM_Record), 100000, &AMSlab) != 0) {
		fprintf(stderr, "Slab init failed.\n");
		exit(1);
	}

	setup_out_context(argv[1], &cbf);

	AMDatabase_init(&AMDb);

	load_AMDatabase_from_AM_file(&AMDb, AMSlab, argv[2]);
	load_AMDatabase_from_HD_file(&AMDb, AMSlab, argv[3]);
	process_and_dump_EN_file(&cbf, &AMDb, argv[4]);

	close_out_context(&cbf);

	slab_free(AMSlab);

	return 0;
}

static void
setup_out_context(const char *outdir, struct callbook_out_context *idx)
{
	char path[PATH_MAX];
	char area, suffix;
	FILE *fp;

	/*
	 * The BBS and the canonical make_cb executable disagree
	 * on the exact semantics of the callbook record size. For the
	 * time being, they appear to ultimately end up with the same size,
	 * but we will check that here.
	 *
	 * The BBS uses a hardcoded size that is in the CPP symbol
	 * "SizeofCALLBK". The make_cb tool, however, uses the size of
	 * the "callbook_entry" structure.
	 *
	 * Likewise, for index entries, the BBS uses "SizeofCBINDEX" whilst
	 * the population tools use sizeof(callbook_index).
	 */
	if (sizeof(struct callbook_entry) != SizeofCALLBK) {
		fprintf(stderr, "Callbook record sizes don't match!\n");
		fprintf(stderr, "Can't proceed.\n");
		exit(2);
	}

	if (sizeof(struct callbook_index) != SizeofCBINDEX) {
		fprintf(stderr, "Callbook index record sizes don't match!\n");
		fprintf(stderr, "Can't proceed.\n");
		exit(2);
	}

	snprintf(path, sizeof(path)-1, "%s/city.indx", outdir);
	if ((idx->fp_city = fopen(path, "w")) == NULL) {
		fprintf(stderr, "Can't open '%s'.\n", path);
		exit(2);
	}

	snprintf(path, sizeof(path)-1, "%s/firstname.indx", outdir);
	if ((idx->fp_first = fopen(path, "w")) == NULL) {
		fprintf(stderr, "Can't open '%s'.\n", path);
		exit(2);
	}

	snprintf(path, sizeof(path)-1, "%s/lastname.indx", outdir);
	if ((idx->fp_last = fopen(path, "w")) == NULL) {
		fprintf(stderr, "Can't open '%s'.\n", path);
		exit(2);
	}

	snprintf(path, sizeof(path)-1, "%s/zip.indx", outdir);
	if ((idx->fp_zip = fopen(path, "w")) == NULL) {
		fprintf(stderr, "Can't open '%s'.\n", path);
		exit(2);
	}

	/*
	 * Create the sub-directories.
	 */

	/* XXX: ASCII dependent. */
	for (area = '0'; area <= '9'; area++) {
		snprintf(path, sizeof(path)-1, "%s/%c", outdir, area);
		if (mkdir(path, 0777) != 0) {
			fprintf(stderr, "Can't create '%s'.\n", path);
			exit(2);
		}

		for (suffix = 'A'; suffix <= 'Z'; suffix++) {
			snprintf(path, sizeof(path)-1, "%s/%c/%c", outdir,
				area, suffix);
			fp = fopen(path, "w");
			if (fp == NULL) {
				fprintf(stderr, "Can't create '%s'.\n", path);
				exit(2);
			}
			fclose(fp);
		}
	}

	idx->outdir = outdir;
}

static void
close_out_context(struct callbook_out_context *cbf)
{
	fclose(cbf->fp_zip);
	fclose(cbf->fp_last);
	fclose(cbf->fp_first);
	fclose(cbf->fp_city);
}

static void
warn_skip(const char *prefix, size_t lineno, const char *reason)
{
	printf("%s:Line %d skipped. (%s)\n", prefix, lineno, reason);
}

static void
warn_skip_ex(const char *prefix, size_t lineno, const char *reason,
	const char *sample)
{
	printf("%s:Line %d skipped. (%s '%s')\n", prefix, lineno, reason,
		sample);
}

static void
load_AMDatabase_from_AM_file(struct AMDatabase *AMDb, slab_t AMSlab,
	const char *path)
{
	char line[128], *field, *iter, *callsign;
	struct ULS_AM_Record *AM;
	uint8_t op_class;
	uint32_t op_call_id;
	int lineno, recno;
	size_t len;
	FILE *fp;

	/*
	 * Load the information from the AM table.
	 */
	fp = fopen(path, "rt");
	if (fp == NULL) {
		fprintf(stderr, "Can't open AM file.\n");
		exit(1);
	}

	lineno = 0;
	recno = 0;

	while (fgets(line, sizeof(line)-1, fp)) {
		if ((lineno & 0xffff) == 0) {
			printf("AM:%07d lines processed\r", lineno);
			fflush(stdout);
		}

		lineno++;
		iter = line;

		field = strsep(&iter, "|");
		if (strcmp(field, "AM") != 0) {
			warn_skip("AM", lineno, "no AM");
			continue;
		}

		strsep(&iter, "|"); /* Seq number */
		strsep(&iter, "|"); /* ULS number */
		strsep(&iter, "|"); /* EBF number */

		callsign = strsep(&iter, "|"); /* Callsign */
		if (callsign == NULL) {
			warn_skip("AM", lineno, "no callsign");
			continue;
		}

		if (call2id(callsign, &op_call_id) != 0) {
			warn_skip_ex("AM", lineno, "bad callsign", callsign);
			continue;
		}

		field = strsep(&iter, "|"); /* Operator class */
		if (field == NULL) {
			warn_skip("AM", lineno, "no class");
			continue;
		}

		if (*field == '\0')
			/* Empty class usually means a club call */
			/* We don't care about such calls */
			continue;

		op_class = (uint8_t) *field;

		AM = (struct ULS_AM_Record *) slab_alloc(AMSlab);
		if (AM == NULL) {
			fprintf(stderr, "AM:Out of memory at line %d.\n",
				lineno);
			exit(1);
		}

		AM->op_call_id = op_call_id;
		AM->op_class = op_class;
		AM->op_expire_year = 0;
		AM->op_expire_month = 0;
		AM->op_expire_day = 0;

		AMDatabase_add(AMDb, AM);

		recno++;
	}

	printf("AM:Processed %d lines, %d records.\n", lineno, recno);

	fclose(fp);
}

static void
load_AMDatabase_from_HD_file(struct AMDatabase *AMDb, slab_t AMSlab,
	const char *path)
{
	char line[1024], *field, *iter, *int_end, *callsign;
	struct ULS_AM_Record *AM;
	uint32_t op_call_id;
	int op_expire_month, op_expire_day, op_expire_year;
	int lineno, recno, res;
	size_t len;
	FILE *fp;

	/*
	 * Load the information from the HD table.
	 */
	fp = fopen(path, "rt");
	if (fp == NULL) {
		fprintf(stderr, "Can't open HD file.\n");
		exit(1);
	}

	lineno = 0;
	recno = 0;

	while (fgets(line, sizeof(line)-1, fp)) {
		if ((lineno & 0xffff) == 0) {
			printf("HD:%07d lines processed\r", lineno);
			fflush(stdout);
		}

		lineno++;
		iter = line;


		field = strsep(&iter, "|");
		if (strcmp(field, "HD") != 0) {
			warn_skip("HD", lineno, "no HD");
			continue;
		}

		strsep(&iter, "|"); /* Seq number */
		strsep(&iter, "|"); /* ULS number */
		strsep(&iter, "|"); /* EBF number */

		callsign = strsep(&iter, "|"); /* Callsign */
		if (callsign == NULL) {
			warn_skip("HD", lineno, "no callsign");
			continue;
		}

		if (call2id(callsign, &op_call_id) != 0) {
			warn_skip_ex("HD", lineno, "bad callsign", callsign);
			continue;
		}

		strsep(&iter, "|"); /* License Status */
		strsep(&iter, "|"); /* Radio Service Code */
		strsep(&iter, "|"); /* Grant Date */

		field = strsep(&iter, "|"); /* Expire Date */
		if (field == NULL) {
			warn_skip("HD", lineno, "no expire");
			continue;
		}
		if (*field == '\0')
			/* Empty for whatever reason */
			continue;

		res = sscanf(field, "%d/%d/%d",
			&op_expire_day, &op_expire_month, &op_expire_year);

		if (res != 3) {
			/* Parsing problem */
			warn_skip_ex("HD", lineno, "bad expire date", field);
			continue;
		}

		/*
		 * Now lookup the cached record for this callsign.
		 * (It should be in our cache because we made a pass to
		 * gather the licensee class from the AM file).
		 */
		res = AMDatabase_lookup_by_call_id(AMDb, op_call_id, &AM);
		if (res != 0) {
			/*
			 * No information about this call, might be expired
			 * or belong to a club.
			 */
			continue;
		}

		AM->op_expire_year = op_expire_year;
		AM->op_expire_month = op_expire_month;
		AM->op_expire_day = op_expire_day;

		recno++;
	}

	printf("HD:Processed %d lines, %d records.\n", lineno, recno);

	fclose(fp);
}

static void
process_and_dump_EN_file(struct callbook_out_context *cbf,
	struct AMDatabase *AMDb, const char *ENpath)
{
	char line[1024]; /* An EN record can be over 600 characters long */
	char *callsign, *iter, *fname, *field, *mi, *lname, *city, *state2;
	char *street_addr, *zip, *suffix;
	size_t len, lineno, recno;
	uint32_t op_call_id;
	char path[PATH_MAX];
	int res;
	FILE *fp;
	struct ULS_AM_Record *AM;

	/*
	 * Load the information from the EN table.
	 */
	fp = fopen(ENpath, "rt");
	if (fp == NULL) {
		fprintf(stderr, "Can't open EN file.\n");
		exit(1);
	}

	lineno = 0;
	recno = 0;

	while (fgets(line, sizeof(line)-1, fp) != NULL) {
		if ((lineno & 0xffff) == 0) {
			printf("EN:%07d lines processed.\r", lineno);
			fflush(stdout);
		}

		lineno++;
		iter = line;

		field = strsep(&iter, "|");
		if (strcmp(field, "EN") != 0) {
			warn_skip("EN", lineno, "no EN");
			continue;
		}

		strsep(&iter, "|"); /* Sequence number */
		strsep(&iter, "|"); /* ULS number */
		strsep(&iter, "|"); /* EBF number */

		callsign = strsep(&iter, "|"); /* Callsign */
		if (callsign == NULL) {
			warn_skip("EN", lineno, "no callsign");
			continue;
		}

		if (call2id(callsign, &op_call_id) != 0) {
			warn_skip_ex("EN", lineno, "bad callsign", callsign);
			continue;
		}

		strsep(&iter, "|"); /* Entity Type */
		strsep(&iter, "|"); /* Licensee ID */
		strsep(&iter, "|"); /* Entity Name */

		fname = strsep(&iter, "|"); /* First Name */
		if (fname == NULL) {
			warn_skip("EN", lineno, "no first name");
			continue;
		}

		mi = strsep(&iter, "|"); /* Middle Initial */
		if (mi == NULL) {
			warn_skip("EN", lineno, "no MI");
			continue;
		}

		lname = strsep(&iter, "|"); /* Last Name */
		if (lname == NULL) {
			warn_skip("EN", lineno, "no last name");
			continue;
		}

		suffix = strsep(&iter, "|"); /* Suffix */
		if (suffix == NULL) {
			warn_skip("EN", lineno, "no suffix");
			continue;
		}

		strsep(&iter, "|"); /* Phone */
		strsep(&iter, "|"); /* Fax */
		strsep(&iter, "|"); /* Email */

		street_addr = strsep(&iter, "|"); /* Street Address */
		if (street_addr == NULL) {
			warn_skip("EN", lineno, "no street address");
			continue;
		}

		city = strsep(&iter, "|"); /* City */
		if (city == NULL) {
			warn_skip("EN", lineno, "no city");
			continue;
		}

		state2 = strsep(&iter, "|"); /* State */
		if (state2 == NULL) {
			warn_skip("EN", lineno, "no state");
			continue;
		}

		zip = strsep(&iter, "|"); /* ZIP Code */
		if (state2 == NULL) {
			warn_skip("EN", lineno, "no state");
			continue;
		}

		/*
		 * Fetch cached data from other files.
		 */
		res = AMDatabase_lookup_by_call_id(AMDb, op_call_id, &AM);
		if (res != 0) {
			/* No data available from the other tables */
			continue;
		}

		add_call(cbf,
			op_call_id,
			lname,
			suffix, /* Last name suffix, not call suffix */
			fname,
			mi,
			street_addr,
			city,
			state2,
			zip,
			"", /* Birthdate no longer available in ULS */
			AM->op_expire_year,
			AM->op_expire_month, /* 1 = Jan */
			AM->op_expire_day,   /* 1 = 1st */
			AM->op_class
		);
	}

}

static void
add_call(struct callbook_out_context *cbf,
	uint32_t callid,
	const char *lname,
	const char *lname_suffix, /* Not used yet */
	const char *fname,
	const char *mname,
	const char *addr,
	const char *city,
	const char *state,
	const char *zip,
	const char *birth,
	uint16_t expire_year,
	uint8_t expire_month, /* 1 = January */
	uint8_t expire_day, /* 1 = 1st */
	char op_class
)
{
	struct callbook_entry entry;
	struct callbook_index idx;
	char path[PATH_MAX], suffix_buf[10], prefix_buf[10], expire[10];
	char area_char, *suffix, *prefix;
	FILE *fp;
	off_t rec_pos;
	int day_of_year;

	suffix = id2suffix(callid, suffix_buf);
	prefix = id2prefix(callid, prefix_buf);
	area_char = id2region(callid) + '0';

	/*
	 * Manufacture the name of the fragment file to which this record
	 * belongs.
	 */
	snprintf(path, sizeof(path)-1, "%s/%c/%c", cbf->outdir, area_char,
		suffix[0]);

	/*
	 * Attempt to open the fragment, for appending.
	 */
	fp = fopen(path, "a");
	if (fp == NULL) {
		fprintf(stderr, "Can't open '%s'.\n", path);
		exit(2);
	}

	/*
	 * Initialize and normalize the record from the data.
	 */
	memset(&entry, 0, sizeof(entry));

	/*
	 * Format the specialized expiration date.
	 */
	day_of_year = get_doy(expire_year, expire_month, expire_day);

	snprintf(expire, sizeof(expire), "%02d%03d",
		expire_year % 100, day_of_year);

	entry.callarea[0] = area_char;
	strncpy(entry.prefix, prefix, sizeof(entry.prefix));
	strncpy(entry.suffix, suffix, sizeof(entry.suffix));
	strncpy_upr(entry.lname, lname, sizeof(entry.lname));
	strncpy_upr(entry.fname, fname, sizeof(entry.fname));
	strncpy_upr(entry.mname, mname, sizeof(entry.mname));
	strncpy_upr(entry.addr, addr, sizeof(entry.addr));
	strncpy_upr(entry.city, city, sizeof(entry.city));
	strncpy_upr(entry.state, state, sizeof(entry.state));
	strncpy_upr(entry.zip, zip, sizeof(entry.zip));
	strncpy_upr(entry.birth, birth, sizeof(entry.birth));
	strncpy_upr(entry.exp, expire, sizeof(entry.exp));
	entry.class[0] = toupper(op_class); /* Man, that is badly named */

	/*
	 * Record the file position at which this record resides. We
	 * will need this when writing information about the record to
	 * the various indexes.
	 */
	rec_pos = ftell(fp);

	/*
	 * Make sure the record offset is reportable in the index
	 * framework that we've set up.
	 */
	if (rec_pos > 0xFFFFFFFFUL) {
		fprintf(stderr, "Index location overflow for '%s'.\n", path);
		exit(2);
	}

	/*
	 * Append the new record to the fragment file.
	 */
	if (fwrite(&entry, sizeof(entry), 1, fp) != 1) {
		fprintf(stderr, "Can't append to '%s'.\n", path);
		exit(2);
	}

	fclose(fp);

	/*
	 * Now append index records to the various indecies.
	 */
	memset(&idx, 0, sizeof(idx));

	/* Encode the record's position */
	idx.loc_xdr[0] = (uint8_t)((rec_pos >> 24) & 0xff);
	idx.loc_xdr[1] = (uint8_t)((rec_pos >> 16) & 0xff);
	idx.loc_xdr[2] = (uint8_t)((rec_pos >>  8) & 0xff);
	idx.loc_xdr[3] = (uint8_t)((rec_pos      ) & 0xff);

	idx.area = area_char;
	idx.suffix = entry.suffix[0];

	/* The last name index */
	strncpy(idx.key, entry.lname, sizeof(idx.key)-1);
	if (fwrite(&idx, sizeof(idx), 1, cbf->fp_last) != 1) {
		fprintf(stderr, "Can't append to Last Name index.\n");
		exit(2);
	}

	/* The first name index */
	strncpy(idx.key, entry.fname, sizeof(idx.key)-1);
	if (fwrite(&idx, sizeof(idx), 1, cbf->fp_first) != 1) {
		fprintf(stderr, "Can't append to First Name index.\n");
		exit(2);
	}

	/* The city index */
	strncpy(idx.key, entry.city, sizeof(idx.key)-1);
	if (fwrite(&idx, sizeof(idx), 1, cbf->fp_city) != 1) {
		fprintf(stderr, "Can't append to City index.\n");
		exit(2);
	}

	/* The zip code index */
	strncpy(idx.key, entry.zip, sizeof(idx.key)-1);
	if (fwrite(&idx, sizeof(idx), 1, cbf->fp_zip) != 1) {
		fprintf(stderr, "Can't append to ZIP index.\n");
		exit(2);
	}
}

static void
test(struct AMDatabase *AMDb)
{
	char line[128], *callsign;
	struct ULS_AM_Record *AM;
	int region;
	size_t len;

	do {
		printf("Callsign: ");
		if (fgets(line, sizeof(line)-1, stdin) == NULL)
			break;

		len = strlen(line);
		if (len < 1)
			break;

		line[len-1] = '\0';

		switch (AMDatabase_lookup_by_callsign(AMDb, line, &AM)) {
		case 0:
			printf("--- FOUND\n");
			callsign = id2call(AM->op_call_id, line);
			region = id2region(AM->op_call_id);
			printf("Callsign: %s\n", callsign);
			printf("Region  : %d\n", region);
			printf("Class   : %c\n", AM->op_class);
			printf("Expire  : %04d-%02d-%02d\n",
				AM->op_expire_year,
				AM->op_expire_month,
				AM->op_expire_day);
			break;
		case -1:
			printf("--- Bad callsign\n");
			break;
		case -2:
			printf("--- NOT FOUND\n");
			break;
		default:
			printf("--- ?\n");
			break;
		}
	} while (1);
}

static int
strncpy_upr(char *dst, const char *src, size_t max)
{
	size_t i;

	for (i = 0; i < max-1; i++) {
		dst[i] = toupper(src[i]);
		if (dst[i] == 0)
			goto Terminated;
	}

	dst[i] = 0;

Terminated:
	return i;
}

/*
 * N0ARY's callbook format stores dates as year (without century) and day of
 * year.
 *
 * year - Canonical year, CE. (1990 CE = 1990)
 * month - Canonical month number (1 = January)
 * day - Canonical day number (1 = first day of the month)
 */
static uint16_t
get_doy(unsigned int year, uint8_t month, uint8_t day)
{
	/*
	 * Given a month in the year (0 = January), return the day
	 * of the year in which the month's first day starts, under a non-
	 * leap year.
	 */
	static const int days[12] = {
		/* Jan */ 0,
		/* Feb */ 31,
		/* Mar */ 31+28,
		/* Apr */ 31+28+31,
		/* May */ 31+28+31+30,
		/* Jun */ 31+28+31+30+31,
		/* Jul */ 31+28+31+30+31+30,
		/* Aug */ 31+28+31+30+31+30+31,
		/* Sep */ 31+28+31+30+31+30+31+31,
		/* Oct */ 31+28+31+30+31+30+31+31+30,
		/* Nov */ 31+28+31+30+31+30+31+31+30+31,
		/* Dec */ 31+28+31+30+31+30+31+31+30+31+30
	};
	int is_leap, doy;

	if ((year % 400) == 0)
		is_leap = 1;
	else if ((year % 100) == 0)
		is_leap = 0;
	else if ((year % 4) == 0)
		is_leap = 1;
	else
		is_leap = 0;

	doy = days[month-1] + day - 1;

	if (is_leap && month >= 3)
		/* Account for February having 29 days in a leap year */
		/* instead of the encoded number, which is 28.        */
		doy += 1;

	return doy;
}

static void
usage(const char *prog)
{
	fprintf(stderr, "usage: %s <outdir> <AM.dat> <HD.dat> <EN.dat>\n", prog);
	fprintf(stderr, "Callbook processor for FCC ULS Database files.\n");
}

