#ifndef _TNCD_KISS_PCAP_H
#define _TNCD_KISS_PCAP_H

#include <stdint.h>

/* Tcpdump.org PCAP type */
#define LINKTYPE_AX25_KISS	202

/*
 * The PCAP file header. Using clever runtime checking of the
 * magic number field, this structure can be written to disk
 * directly without care of the native CPU byte order.
 */
struct pcap_header {
	uint32_t magic_number;
	uint16_t version_major;
	uint16_t version_minor;
	int32_t	 this_zone;
	uint32_t sigfigs;
	uint32_t snaplen;
	uint32_t network;
};

struct pcap_packet_header_ns {
	uint32_t tm_s;
	uint32_t tm_ns;
	uint32_t packet_length;
	uint32_t capture_length;
};

#endif
