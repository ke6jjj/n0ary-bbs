#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "slab.h"
#include "am_database.h"
#include "am_record.h"
#include "call_id.h"
#include "bbslib.h"

static void usage(const char *prog);
static void load_AMDatabase_from_AM_file(struct AMDatabase *AMDb,
	slab_t AMSlab, const char *path);
static void load_AMDatabase_from_HD_file(struct AMDatabase *AMDb,
	slab_t AMSlab, const char *path);
static void test(struct AMDatabase *AMDb);

int
main(int argc, char *argv[])
{
	slab_t AMSlab;
	struct AMDatabase AMDb;

	if (argc < 5) {
		usage(argv[0]);
		exit(1);
	}

	if (slab_init(sizeof(struct ULS_AM_Record), 100000, &AMSlab) != 0) {
		fprintf(stderr, "Slab init failed.\n");
		exit(1);
	}

	AMDatabase_init(&AMDb);

	load_AMDatabase_from_AM_file(&AMDb, AMSlab, argv[2]);
	load_AMDatabase_from_HD_file(&AMDb, AMSlab, argv[3]);

	test(&AMDb);

#if 0
	process_and_dump_EN_file(&AMDb, argv[1], argv[4]);
#endif

	slab_free(AMSlab);

	return 0;
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
			printf("AM:Line %d skipped. (no AM)\n", lineno);
			continue;
		}

		field = strsep(&iter, "|"); /* Seq number */
		field = strsep(&iter, "|"); /* ULS number */
		field = strsep(&iter, "|"); /* EBF number */

		field = strsep(&iter, "|"); /* Callsign */
		if (field == NULL) {
			printf("AM:Line %d skipped (no callsign).\n", lineno);
			continue;
		}

		if (call2id(field, &op_call_id) != 0) {
			printf("AM:Line %d skipped (bad callsign '%s').\n",
				lineno, field);
			continue;
		}

		callsign = field;

		field = strsep(&iter, "|"); /* Operator class */
		if (field == NULL || *field == '\0') {
			//printf("AM:Line %d skipped (no class).\n", lineno);
			continue;
		}

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
			printf("HD:Line %d skipped. (no HD)\n", lineno);
			continue;
		}

		field = strsep(&iter, "|"); /* Seq number */
		field = strsep(&iter, "|"); /* ULS number */
		field = strsep(&iter, "|"); /* EBF number */

		field = strsep(&iter, "|"); /* Callsign */
		if (field == NULL) {
			printf("HD:Line %d skipped (no callsign).\n", lineno);
			continue;
		}

		if (call2id(field, &op_call_id) != 0) {
			printf("HD:Line %d skipped (bad callsign '%s').\n",
				lineno, field);
			continue;
		}

		callsign = field;

		field = strsep(&iter, "|"); /* License Status */
		field = strsep(&iter, "|"); /* Radio Service Code */
		field = strsep(&iter, "|"); /* Grant Date */
		field = strsep(&iter, "|"); /* Expire Date */

		if (field == NULL) {
			printf("HD:Line %d skipped (no expire).\n", lineno);
			continue;
		}

		res = sscanf(field, "%d/%d/%d",
			&op_expire_day, &op_expire_month, &op_expire_year);

		if (res != 3) {
			/* Parsing problem */
			//printf("HD:Line %d skipped (bad expire date '%s')\n",
			//	lineno, field);
			continue;
		}

		/*
		 * Now lookup the cached record for this callsign.
		 * (It should be in our cache because we made a pass to
		 * gather the licensee class from the AM file).
		 */
		res = AMDatabase_lookup_by_call_id(AMDb, op_call_id, &AM);
		if (res != 0) {
		//	printf("HD:Line %d skipped; no existing information "
		//		"about callsign '%s'\n", lineno, callsign);
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
test(struct AMDatabase *AMDb)
{
	char line[128], *callsign;
	struct ULS_AM_Record *AM;
	size_t len;
	int region;

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

static void
usage(const char *prog)
{
	fprintf(stderr, "usage: %s <outdir> <AM.dat> <HD.dat> <EN.dat>\n", prog);
	fprintf(stderr, "Callbook processor for FCC ULS Database files.\n");
}
