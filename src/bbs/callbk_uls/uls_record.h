/*
 * Routines and structure for representing an FCC Amateur record from
 * a ULS database.
 */
#ifndef MAKECB_ULS_RECORD_H
#define MAKECB_ULS_RECORD_H

#include <stdint.h>

struct ULS_Record {
	struct ULS_Record *hash_next;
	uint32_t op_uls_id;
	uint8_t  op_class;
	uint8_t  op_expire_month;
	uint8_t  op_expire_day;
	uint16_t op_expire_year;
};

#endif
