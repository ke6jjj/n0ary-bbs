#ifndef AGWPE_H
#define AGWPE_H

/*
 * The fixed header that appears on all AGWPE frames, be they from
 * AGWPE or the application.
 */
struct agwpe_hdr_wire {
	uint8_t port;
	uint8_t reserved_0[3];
	uint8_t data_kind;
	uint8_t reserved_1;
	uint8_t pid;
	uint8_t reserved_2;
	uint8_t call_from[10];
	uint8_t call_to[10];
	uint8_t data_len[4];
	uint8_t reserved_3[4];
};

/*
 * In-memory representation of the same, minus unused elements, and decoded
 * appropriately for the host.
 */
struct agwpe_hdr {
	uint8_t port;
	uint8_t data_kind;
	uint8_t pid;
	uint8_t call_from[10];
	uint8_t call_to[10];
	size_t  data_len;
};

#endif
