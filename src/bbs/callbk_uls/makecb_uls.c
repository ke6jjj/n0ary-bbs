#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "slab.h"
#include "am_database.h"
#include "am_record.h"
#include "call_id.h"

int
main(int argc, char *argv[])
{
	char line[128], *field, *iter, *region_end, *callsign;
	slab_t AMSlab;
	int lineno, recno;
	uint32_t op_call_id;
	uint8_t op_class, op_region;
	struct ULS_AM_Record *AM;
	struct AMDatabase AMDb;
	size_t len;
	FILE *fp;

	if (argc < 2) {
		fprintf(stderr, "usage: db <AM.dat>\n");
		exit(1);
	}

	fp = fopen(argv[1], "rt");
	if (fp == NULL) {
		fprintf(stderr, "Can't open AM file.\n");
		exit(1);
	}

	if (slab_init(sizeof(struct ULS_AM_Record), 100000, &AMSlab) != 0) {
		fprintf(stderr, "Slab init failed.\n");
		exit(1);
	}

	AMDatabase_init(&AMDb);

	lineno = 0;
	recno = 0;

	while (fgets(line, sizeof(line)-1, fp)) {
		lineno++;
		iter = line;

		field = strsep(&iter, "|");
		if (strcmp(field, "AM") != 0) {
			printf("Line %d skipped. (no AM)\n", lineno);
			continue;
		}

		field = strsep(&iter, "|"); /* Seq number */
		field = strsep(&iter, "|"); /* ULS number */
		field = strsep(&iter, "|"); /* EBF number */

		field = strsep(&iter, "|"); /* Callsign */
		if (field == NULL) {
			printf("Line %d skipped (no callsign).\n", lineno);
			continue;
		}

		if (call2id(field, &op_call_id) != 0) {
			printf("Line %d skipped (bad callsign '%s').\n",
				lineno, field);
			continue;
		}

		callsign = field;

		field = strsep(&iter, "|"); /* Operator class */
		if (field == NULL) {
			printf("Line %d skipped (no class).\n", lineno);
			continue;
		}

		op_class = (uint8_t) *field;

		field = strsep(&iter, "|"); /* Group code */

		field = strsep(&iter, "|"); /* Region code */
		if (field == NULL) {
			printf("Line %d skipped (no region).\n", lineno);
			continue;
		}

		op_region = (uint8_t) strtoul(field, &region_end, 10);
		if (region_end == field || *region_end != '\0') {
			printf("Line %d skipped (bad region num '%s') "
				"(callsign %s).\n",
				lineno, field, callsign);
			continue;
		}

		AM = (struct ULS_AM_Record *) slab_alloc(AMSlab);
		if (AM == NULL) {
			fprintf(stderr, "Out of memory at line %d.\n", lineno);
			exit(1);
		}

		AM->op_call_id = op_call_id;
		AM->op_region = op_region;
		AM->op_class = op_class;

		AMDatabase_add(&AMDb, AM);

		recno++;
	}

	printf("Processed %d lines, %d records.\n", lineno, recno);

	fclose(fp);

	do {
		printf("Callsign: ");
		if (fgets(line, sizeof(line)-1, stdin) == NULL)
			break;

		len = strlen(line);
		if (len < 1)
			break;

		line[len-1] = '\0';

		switch (AMDatabase_lookup_by_callsign(&AMDb, line, &AM)) {
		case 0:
			printf("--- FOUND\n");
			field = id2call(AM->op_call_id, line);
			printf("Callsign: %s\n", field);
			printf("Class   : %c\n", AM->op_class);
			printf("Region  : %d\n", AM->op_region);
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

	slab_free(AMSlab);

	return 0;
}
