#include <stddef.h>
#include <stdint.h>

#include "uls_database.h"
#include "uls_record.h"

void
ULSDatabase_init(struct ULSDatabase *db)
{
	size_t i;

	for (i = 0; i < ULSDatabase_Hash_Key; i++)
		db->bucket[i] = NULL;
}

void
ULSDatabase_add(struct ULSDatabase *db, struct ULS_Record *rec)
{
	size_t bucket = rec->op_uls_id % ULSDatabase_Hash_Key;

	rec->hash_next = db->bucket[bucket];
	db->bucket[bucket] = rec;
}

int
ULSDatabase_lookup_by_uls_id(struct ULSDatabase *db, uint32_t uls_id,
	struct ULS_Record **res)
{
	size_t bucket;
	struct ULS_Record *rec;

	bucket = uls_id % ULSDatabase_Hash_Key;
	for (rec = db->bucket[bucket]; rec != NULL; rec = rec->hash_next) {
		if (rec->op_uls_id == uls_id) {
			*res = rec;
			return 0;
		}
	}

	return -2; /* Not found */
}
