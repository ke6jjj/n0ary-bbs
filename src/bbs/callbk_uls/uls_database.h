#ifndef MAKECB_ULS_DATABASE_H
#define MAKECB_ULS_DATABASE_H

#define ULSDatabase_Hash_Key 10243

struct ULSDatabase {
	struct ULS_Record *bucket[ULSDatabase_Hash_Key];
};

void ULSDatabase_init(struct ULSDatabase *db);
void ULSDatabase_add(struct ULSDatabase *db, struct ULS_Record *amr);
int  ULSDatabase_lookup_by_uls_id(struct ULSDatabase *db, uint32_t uls_id,
	struct ULS_Record **res);

#endif
