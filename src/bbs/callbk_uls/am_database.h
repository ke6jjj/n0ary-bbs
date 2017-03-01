#ifndef MAKECB_ULS_AM_DATABASE_H
#define MAKECB_ULS_AM_DATABASE_H

#define AMDatabase_Hash_Key 1023

struct AMDatabase {
	struct ULS_AM_Record *bucket[AMDatabase_Hash_Key];
};

void AMDatabase_init(struct AMDatabase *db);
void AMDatabase_add(struct AMDatabase *db, struct ULS_AM_Record *amr);
int AMDatabase_lookup_by_callsign(struct AMDatabase *db, const char *call,
	struct ULS_AM_Record **res);
int AMDatabase_lookup_by_call_id(struct AMDatabase *db, uint32_t call,
	struct ULS_AM_Record **res);

#endif
