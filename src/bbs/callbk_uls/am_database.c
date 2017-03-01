#include <stddef.h>
#include <stdint.h>

#include "am_database.h"
#include "am_record.h"
#include "call_id.h"

void
AMDatabase_init(struct AMDatabase *db)
{
	size_t i;

	for (i = 0; i < AMDatabase_Hash_Key; i++)
		db->bucket[i] = NULL;
}

void
AMDatabase_add(struct AMDatabase *db, struct ULS_AM_Record *amr)
{
	size_t bucket = amr->op_call_id % AMDatabase_Hash_Key;

	amr->hash_next = db->bucket[bucket];
	db->bucket[bucket] = amr;
}

int
AMDatabase_lookup_by_callsign(struct AMDatabase *db, const char *call,
	struct ULS_AM_Record **res)
{
	uint32_t call_id;
	size_t bucket;
	struct ULS_AM_Record *rec;

	if (call2id(call, &call_id) != 0)
		return -1; /* Bad callsign */

	bucket = call_id % AMDatabase_Hash_Key;
	for (rec = db->bucket[bucket]; rec != NULL; rec = rec->hash_next) {
		if (rec->op_call_id == call_id) {
			*res = rec;
			return 0;
		}
	}

	return -2; /* Not found */
}
