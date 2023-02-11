/*
 * Copyright 2022, Jeremy Cooper.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Jeremy Cooper.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * This program was written based on specifications in the FCC
 * ULS data file formats as found in a document titled "ULS Data File Formats"
 * from the FCC website as of January, 2017.
 *
 * In 2022 it was upgraded to additionally process amateur records from
 * Innovation, Science and Economic Development Canada.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>

#include "slab.h"
#include "uls_database.h"
#include "uls_record.h"
#include "field_proc.h"
#include "AM_proc.h"
#include "HD_proc.h"
#include "EN_proc.h"
#include "CA_proc.h"
#include "cb_date.h"
#include "call_id.h"

#include "bbslib.h"
#include "callbk.h" /* From BBS, for checking sizes */

/*
 * The details of a callbook generation session.
 */
struct callbook_out_context {
	FILE *fp_city;
	FILE *fp_first;
	FILE *fp_last;
	FILE *fp_zip;
	const char *outdir;
};

/*
 * A description of a record field and a function for processing it.
 */
struct Record_Field_Process_struct {
	const char    *field_name;
	field_proc_t  field_proc;
};
typedef struct Record_Field_Process_struct Record_Field_Process;

static void usage(const char *prog);
static void setup_out_context(const char *outdir,
	struct callbook_out_context *cbf);
static void close_out_context(struct callbook_out_context *cbf);
static void load_ULSDatabase_from_AM_file(struct ULSDatabase *ULSDb,
	slab_t ULSSlab, const char *path);
static void load_ULSDatabase_from_HD_file(struct ULSDatabase *ULSDb,
	slab_t ULSSlab, const char *path);
static void process_and_dump_EN_file(struct callbook_out_context *cbf,
	struct ULSDatabase *ULSDb, const char *ENpath);
static void process_and_dump_CA_file(struct callbook_out_context *cbf,
	const char *CApath);
static void test_query(struct ULSDatabase *ULSDb);
static void warn_skip(const char *prefix, size_t lineno, const char *reason);
static void warn_skip_ex(const char *prefix, size_t lineno, const char *reason,
                        const char *sample);
static void add_call(struct callbook_out_context *,
	uint32_t callid,
	const char *entity_name,
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
	char op_class,
	uint8_t use_entity_name
);
static int strncpy_upr(char *dst, const char *src, size_t max);
static int process_record(const Record_Field_Process *fields,
	size_t num_fields, void *ctx, const char *warn_prefix,
	size_t lineno, char *line, const char *delim);

/*
 * The processing scheme for an 'AM' record.
 */
const Record_Field_Process AM_fields[] = {
	{ "Header",                    AM_proc_header    },
	{ "Unique System Identifier",  AM_proc_uls_id    },
	{ "ULS File Number",           NULL              },
	{ "EBF Number",                NULL              },
	{ "Call Sign",                 NULL              },
	{ "Operator Class",            AM_proc_op_class  },
};
const int AM_fields_count = sizeof(AM_fields) / sizeof(AM_fields[0]);

/*
 * The processing scheme for an 'HD' record.
 */
const Record_Field_Process HD_fields[] = {
	{ "Header",                    HD_proc_header         },
	{ "Unique System Identifier",  HD_proc_uls_id         },
	{ "ULS File Number",           NULL                   },
	{ "EBF Number",                NULL                   },
	{ "Call Sign",                 NULL                   },
	{ "License Status",            HD_proc_license_status },
	{ "Radio Service Code",        NULL                   },
	{ "Grant Date",                NULL                   },
	{ "Expired Date",              HD_proc_expire_date    },
};
const int HD_fields_count = sizeof(HD_fields) / sizeof(HD_fields[0]);

/*
 * The processing scheme for an 'EN' record.
 */
const Record_Field_Process EN_fields[] = {
	{ "Header",                    EN_proc_header         },
	{ "Unique System Identifier",  EN_proc_uls_id         },
	{ "ULS File Number",           NULL                   },
	{ "EBF Number",                NULL                   },
	{ "Call Sign",                 EN_proc_call_sign      },
	{ "Entity Type",               NULL                   },
	{ "Licensee ID",               NULL                   },
	{ "Entity Name",               EN_proc_entity_name    },
	{ "First Name",                EN_proc_first_name     },
	{ "MI",                        EN_proc_mi             },
	{ "Last Name",                 EN_proc_last_name      },
	{ "Suffix",                    EN_proc_suffix         },
	{ "Phone",                     NULL                   },
	{ "Fax",                       NULL                   },
	{ "Email",                     NULL                   },
	{ "Street Address",            EN_proc_street_addr    },
	{ "City",                      EN_proc_city           },
	{ "State",                     EN_proc_state          },
	{ "ZIP Code",                  EN_proc_zip            },
};
const int EN_fields_count = sizeof(EN_fields) / sizeof(EN_fields[0]);

/*
 * The processing scheme for a 'CA' record.
 */
const Record_Field_Process CA_fields[] = {
	{ "Call Sign",                 CA_proc_call_sign      },
	{ "First Name",                CA_proc_first_name     },
	{ "Surname",                   CA_proc_surname        },
	{ "Address",                   CA_proc_address        },
	{ "City",                      CA_proc_city           },
	{ "Province",                  CA_proc_province       },
	{ "Postal Code",               CA_proc_postal_code    },
	{ "Quailification A",          CA_proc_qual_a         },
	{ "Quailification B",          CA_proc_qual_b         },
	{ "Quailification C",          CA_proc_qual_c         },
	{ "Quailification D",          CA_proc_qual_d         },
	{ "Quailification E",          CA_proc_qual_e         },
	{ "Club Name",                 CA_proc_club_name      },
	{ "Club Name 2",               CA_proc_club_name2     },
	{ "Club Address",              CA_proc_club_addr      },
	{ "Club City",                 CA_proc_club_city      },
	{ "Club Province",             CA_proc_club_prov      },
	{ "Club Postal Code",          CA_proc_club_postal    },
};
const int CA_fields_count = sizeof(CA_fields) / sizeof(CA_fields[0]);

int
main(int argc, char *argv[])
{
	slab_t ULSSlab;
	struct ULSDatabase ULSDb;
	struct callbook_out_context cbf;

	if (argc < 5) {
		usage(argv[0]);
		exit(1);
	}

	if (slab_init(sizeof(struct ULS_Record), 100000, &ULSSlab) != 0) {
		fprintf(stderr, "Slab init failed.\n");
		exit(1);
	}

	setup_out_context(argv[1], &cbf);

	ULSDatabase_init(&ULSDb);

	load_ULSDatabase_from_AM_file(&ULSDb, ULSSlab, argv[2]);
	load_ULSDatabase_from_HD_file(&ULSDb, ULSSlab, argv[3]);
	process_and_dump_EN_file(&cbf, &ULSDb, argv[4]);
	if (argc > 5) {
		process_and_dump_CA_file(&cbf, argv[5]);
	}

	close_out_context(&cbf);

	printf("\nDone.\n");
	fflush(stdout);

	slab_free(ULSSlab);

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
warn_skip(const char *prefix, size_t lineno, const char *name)
{
	printf("%s:Line %zd skipped. (bad %s)\n", prefix, lineno, name);
}

static void
warn_skip_ex(const char *prefix, size_t lineno, const char *name,
	const char *sample)
{
	printf("%s:Line %zd skipped. (bad %s '%s')\n", prefix, lineno, name,
		sample);
}

static void
warn_skip_missing(const char *prefix, size_t lineno, const char *name)
{
	printf("%s:Line %zd skipped. (delimiter missing at %s).\n", prefix,
		lineno, name);
}

static void
load_ULSDatabase_from_AM_file(struct ULSDatabase *ULSDb, slab_t ULSSlab,
	const char *path)
{
	char line[128];
	int lineno, recno;
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

		/*
		 * Process fields until done.
		 */
		AM_process_ctx ctx;

		int res = process_record(AM_fields, AM_fields_count, &ctx,
			"AM", lineno, line, "|");

		if (res != 0)
			/* Skip this record */
			continue;

		/*
		 * Allocate a new ULS record for our local database.
		 */
		struct ULS_Record *ULS;

		ULS = (struct ULS_Record *) slab_alloc(ULSSlab);
		if (ULS == NULL) {
			fprintf(stderr, "AM:Out of memory at line %d.\n",
				lineno);
			exit(1);
		}

		ULS->op_uls_id = ctx.AM_uls_id;
		ULS->op_class = ctx.AM_op_class;

		/* These fields will be populated from the HD file */
		ULS->op_expire_year = 0;
		ULS->op_expire_month = 0;
		ULS->op_expire_day = 0;

		ULSDatabase_add(ULSDb, ULS);

		recno++;
	}

	printf("AM:Processed %d lines, %d records.\n", lineno, recno);

	fclose(fp);
}

static void
load_ULSDatabase_from_HD_file(struct ULSDatabase *ULSDb, slab_t ULSSlab,
	const char *path)
{
	char line[1024];
	int lineno, recno;
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

		/*
		 * Process fields until done.
		 */
		HD_process_ctx ctx;

		int res = process_record(HD_fields, HD_fields_count, &ctx,
			"HD", lineno, line, "|");

		if (res != 0)
			/* Skip this record */
			continue;

		struct ULS_Record *ULS;

		/*
		 * Now lookup the cached record for this ULS id.
		 * (It should be in our cache because we made a pass to
		 * gather the licensee class from the AM file).
		 */
		res = ULSDatabase_lookup_by_uls_id(ULSDb, ctx.HD_uls_id, &ULS);
		if (res != 0) {
			/*
			 * No information about this ULS ID, might be expired
			 * or belong to a club.
			 */
			continue;
		}

		ULS->op_expire_year = ctx.HD_expire_year;
		ULS->op_expire_month = ctx.HD_expire_month;
		ULS->op_expire_day = ctx.HD_expire_day;

		recno++;
	}

	printf("HD:Processed %d lines, %d records.\n", lineno, recno);

	fclose(fp);
}

static void
process_and_dump_EN_file(struct callbook_out_context *cbf,
	struct ULSDatabase *ULSDb, const char *ENpath)
{
	char line[1024]; /* An EN record can be over 600 characters long */
	int lineno, recno;
	FILE *fp;

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


		/*
		 * Process fields until done.
		 */
		EN_process_ctx ctx;

		int res = process_record(EN_fields, EN_fields_count, &ctx,
			"EN", lineno, line, "|");

		if (res != 0)
			/* Skip this record */
			continue;

		struct ULS_Record *ULS;

		/*
		 * Fetch cached data from other files.
		 */
		res = ULSDatabase_lookup_by_uls_id(ULSDb, ctx.EN_uls_id, &ULS);
		if (res != 0) {
			/* No data available from the other tables */
			continue;
		}

		/*
		 * Make sure the expiration date got filled in.
		 */
		if (ULS->op_expire_year == 0)
			/* Record incomplete, skip */
			continue;

		/* Finally, save the data */
		add_call(cbf,
			ctx.EN_call_id,
			ctx.EN_entity_name,
			ctx.EN_last_name,
			ctx.EN_suffix, /* Last name suffix, not call suffix */
			ctx.EN_first_name,
			ctx.EN_mi,
			ctx.EN_street_addr,
			ctx.EN_city,
			ctx.EN_state,
			ctx.EN_zip,
			"", /* Birthdate no longer available in ULS */
			ULS->op_expire_year,
			ULS->op_expire_month, /* 1 = Jan */
			ULS->op_expire_day,   /* 1 = 1st */
			ULS->op_class,
			ULS->op_class == 'C' ? 1 : 0 /* Use entity name */
		);
	}

}

static void
process_and_dump_CA_file(struct callbook_out_context *cbf, const char *CApath)
{
	char line[1024]; /* A CA record can be over 600 characters long */
	int lineno, recno, has_code, is_club;
	FILE *fp;
	char synth_class;

	/*
	 * Load the information from the Industry Canada file.
	 */
	fp = fopen(CApath, "rt");
	if (fp == NULL) {
		fprintf(stderr, "Can't open CA file.\n");
		exit(1);
	}

	if (fgets(line, sizeof(line)-1, fp) == NULL) {
		fprintf(stderr, "Can't read header from CA file.\n");
		exit(1);
	}

	if (strcmp(line, "callsign;first_name;surname;address_line;city;"
	                 "prov_cd;postal_code;qual_a;qual_b;qual_c;qual_d;"
	                 "qual_e;club_name;club_name_2;club_address;"
	                 "club_city;club_prov_cd;club_postal_code\r\n") != 0) {
		fprintf(stderr, "CA header line unrecognized/changed.\n");
		exit(1);
	}

	lineno = 1;
	recno = 0;

	while (fgets(line, sizeof(line)-1, fp) != NULL) {
		if ((lineno & 0xffff) == 0) {
			printf("CA:%07d lines processed.\r", lineno);
			fflush(stdout);
		}

		lineno++;

		/*
		 * Process fields until done.
		 */
		CA_process_ctx ctx;

		int res = process_record(CA_fields, CA_fields_count, &ctx,
			"CA", lineno, line, ";");

		if (res != 0)
			/* Skip this record */
			continue;

		/*
		 * Industry Canada represents Canadian license classes
		 * as binary bit vectors, labelled A-E. Not all combinations
		 * are valid. This is what I have observed:
		 *
		 *  A B C D E  Class
		 *  - - - - -  -----
		 *  x          Basic
		 *          x  Basic with Honours
		 *    x     x  Basic with Honours with Code
		 *  x     x    Advanced
		 *  x x   x    Advanced with (basic?) Code
		 *  x   x x    Advanced with (advanced?) Code
		 *  x x x x    Advanced with (basic+advanced?) Code 
		 *        x x  Advanced, Basic with Honours
		 *    x   x x  Advanced, Basic with Honours, Code
		 *    x x x x  Advanced with Code
		 *
		 * I choose to represent this mess as just four classes:
		 *   B - Basic 
		 *   C - Basic with Code
		 *   A - Advanced
		 *   E - Advanced with Code
		 */
		has_code = (ctx.CA_qual_b || ctx.CA_qual_c);
		if (ctx.CA_qual_d)
			synth_class = has_code ? 'C' : 'B';
		else
			synth_class = has_code ? 'E' : 'A';
			
		is_club = strcmp(ctx.CA_club_name, "") != 0;

		/* Save the data */
		add_call(cbf,
			ctx.CA_call_id,
			ctx.CA_club_name,
			is_club ? ctx.CA_club_name2  : ctx.CA_surname,
			NULL, /* Last name suffix */
			ctx.CA_first_name,
			is_club ? ""                 : ctx.CA_middle_name,
			is_club ? ctx.CA_club_addr   : ctx.CA_addr_line,
			is_club ? ctx.CA_club_city   : ctx.CA_city,
			is_club ? ctx.CA_club_prov   : ctx.CA_province,
			is_club ? ctx.CA_club_postal : ctx.CA_postal_code,
			"", /* Birthdate no longer available in ULS */
			2099, /* Expire year */
			99,   /* Expire month, 1 = Jan */
			99,   /* Expire day, 1 = 1st */
			synth_class,
			is_club
		);
	}
}

static void
add_call(struct callbook_out_context *cbf,
	uint32_t callid,
	const char *entity_name,
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
	char op_class,
	uint8_t use_entity_name
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

	/*
	 * These fields are single characters.
	 */
	entry.callarea[0] = area_char;
	entry.class[0] = toupper(op_class); /* Man, that is badly named */

	/*
	 * These fields are longer strings. We try to preserve
	 * the casing of most fields.  For State and callsign
	 * prefix and suffix we make certain the data is uppercased.
	 *
	 * (When writing to indexes, however, we will normalize all fields
	 * to uppercase).
	 */
	strncpy_upr(entry.prefix, prefix, sizeof(entry.prefix)-1);
	strncpy_upr(entry.suffix, suffix, sizeof(entry.suffix)-1);
	strncpy    (entry.lname, lname, sizeof(entry.lname)-1);
	strncpy    (entry.mname, mname, sizeof(entry.mname)-1);
	strncpy    (entry.addr, addr, sizeof(entry.addr)-1);
	strncpy    (entry.city, city, sizeof(entry.city)-1);
	strncpy_upr(entry.state, state, sizeof(entry.state)-1);
	strncpy    (entry.zip, zip, sizeof(entry.zip)-1);
	strncpy    (entry.birth, birth, sizeof(entry.birth)-1);
	strncpy    (entry.exp, expire, sizeof(entry.exp)-1);

	/*
	 * Club callsigns don't have first and last names, they have
	 * entity names. Use the entity name as the first name in such
	 * cases.
	 */
	if (use_entity_name)
		strncpy(entry.fname, entity_name, sizeof(entry.fname)-1);
	else
		strncpy(entry.fname, fname, sizeof(entry.fname)-1);

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
	strncpy_upr(idx.key, entry.lname, sizeof(idx.key)-1);
	if (fwrite(&idx, sizeof(idx), 1, cbf->fp_last) != 1) {
		fprintf(stderr, "Can't append to Last Name index.\n");
		exit(2);
	}

	/* The first name index */
	strncpy_upr(idx.key, entry.fname, sizeof(idx.key)-1);
	if (fwrite(&idx, sizeof(idx), 1, cbf->fp_first) != 1) {
		fprintf(stderr, "Can't append to First Name index.\n");
		exit(2);
	}

	/* The city index */
	strncpy_upr(idx.key, entry.city, sizeof(idx.key)-1);
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
test_query(struct ULSDatabase *ULSDb)
{
	char line[128], *uls_id_str;
	u_int32_t uls_id;
	struct ULS_Record *ULS;
	size_t len;

	do {
		printf("ULS ID: ");
		if (fgets(line, sizeof(line)-1, stdin) == NULL)
			break;

		len = strlen(line);
		if (len < 1)
			break;

		line[len-1] = '\0';

		uls_id = strtoul(uls_id_str, NULL, 10);

		switch (ULSDatabase_lookup_by_uls_id(ULSDb, uls_id, &ULS)) {
		case 0:
			printf("--- FOUND\n");
			printf("Class   : %c\n", ULS->op_class);
			printf("Expire  : %04d-%02d-%02d\n",
				ULS->op_expire_year,
				ULS->op_expire_month,
				ULS->op_expire_day);
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

	for (i = 0; i < max; i++) {
		dst[i] = toupper(src[i]);
		if (dst[i] == 0)
			goto Terminated;
	}

	dst[i] = 0;

Terminated:
	return i;
}

/*
 * Process a retrieved record line using the given field parsing scheme
 * and an opaque processing context (which each processing function will
 * know how to handle).
 *
 * Returns 0 if the record has been parsed ok and should continue on
 * for further processing.
 */
static int
process_record(const Record_Field_Process *fields, size_t num_fields,
	void *ctx, const char *warn_prefix, size_t lineno, char *line,
	const char *delim)
{
	char *iter = line;

	/*
	 * Process fields until done.
	 */
	size_t i;

	for (i = 0; i < num_fields; i++) {
		/*
		 * Fetch the next field from the line.
		 */
		const char *field_name = fields[i].field_name;
		char *contents = strsep(&iter, delim);

		/*
		 * Make sure we at least found a field delimiter.
		 */
		if (contents == NULL) {
			/* Stop processing this record */
			warn_skip_missing(warn_prefix, lineno, field_name);
			return -1;
		}

		/*
		 * Process the field, if asked.
		 */
		if (fields[i].field_proc == NULL) {
			/* Nothing needs to be done with this field */
			continue;
		}

		FPStatus status = fields[i].field_proc(ctx, contents);

		/*
		 * Stop processing if something went wrong with the
		 * field or the record just isn't worth looking at.
		 */
		switch (status) {
		case FP_OK:
			/* Field is fine. Continue on. */
			break;
		case FP_Skip_Record:
			/* Skip record, no warniing needed */
			return -1;
		case FP_Skip_Record_Warn:
			/* Skip this record and complain */
			warn_skip(warn_prefix, lineno, field_name);
			return -1;
		case FP_Skip_Record_Warn_Ex:
			/* Skip this record and complain with contents */
			warn_skip_ex(warn_prefix, lineno, field_name, contents);
			return -1;
		case FP_Record_Complete:
			/*
			 * The record is ok to process, no more fields
			 * need to be inspected.
			 */
			return 0;
		default:
			fprintf(stderr, "Unhandled FPStatus.\n");
			exit(3);
		}
	}

	/* Processing complete */
	return 0;
}


static void
usage(const char *prog)
{
	fprintf(stderr, "usage: %s <outdir> <AM.dat> <HD.dat> <EN.dat>"
	                "[<CA.txt>]\n", prog);
	fprintf(stderr, "Callbook processor for FCC ULS Database files.\n");
}

