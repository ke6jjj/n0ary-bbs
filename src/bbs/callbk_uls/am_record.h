/*
 * Routines and structure for representing an FCC Amateur record from
 * a ULS database.
 */
#ifndef MAKECB_ULS_AM_RECORD_H
#define MAKECB_ULS_AM_RECORD_H

#include <stdint.h>

struct ULS_AM_Record {
	struct ULS_AM_Record *hash_next;
	uint32_t op_call_id;
	uint8_t  op_region;
	uint8_t  op_class;
};

#endif
