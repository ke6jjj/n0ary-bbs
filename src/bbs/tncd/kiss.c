#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "tools.h"
#include "mbuf.h"
#include "kiss.h"
#include "slip.h"
#include "timer.h"
#include "ax25.h"
#include "kiss_pcap.h"

/* KISS TNC control */
#define	KISS_DATA	0

struct kiss {
	mbuf_recv_fn send;
	void *send_arg;
	FILE *pcap_dump;
};

static int kiss_write_pcap_header(FILE *fp);
static int kiss_pcap_dump(kiss *kiss, struct mbuf *bp);

kiss *
kiss_init(void)
{
	kiss *kiss = malloc_struct(kiss);
	if (kiss == NULL)
		return NULL;

	kiss->send = NULL;
	kiss->send_arg = NULL;
	kiss->pcap_dump = NULL;

	return kiss;
}

/* Send raw data packet on KISS TNC */
int
kiss_raw(kiss *dev, struct mbuf *data)
{
	struct mbuf *bp;

	ax25_dump(data);

	/* Put type field for KISS TNC on front */
	if((bp = pushdown(data,1)) == NULLBUF){
		free_p(data);
		return 0;
	}
	bp->data[0] = KISS_DATA;

	kiss_pcap_dump(dev, bp);

	return dev->send(dev->send_arg, bp);
}

/* Process incoming KISS TNC frame */
int
kiss_recv(void *devp, struct mbuf *bp)
{
	kiss *dev = (struct kiss *) devp;
	char kisstype;

	kiss_pcap_dump(dev, bp);

	kisstype = pullchar(&bp);
	ax25_dump(bp);
	switch(kisstype & 0xf){
	case KISS_DATA:
		ax_recv(dev, bp);
		break;
	default:
		free_p(bp);
		break;
	}

	return 0;
}

int
kiss_set_send(kiss *kiss, mbuf_recv_fn send_fn, void *send_arg)
{
	kiss->send = send_fn;
	kiss->send_arg = send_arg;
	return 0;
}

int
kiss_set_pcap_dump(kiss *kiss, FILE *fp)
{
	int res;

	if (kiss->pcap_dump != NULL) {
		fclose(kiss->pcap_dump);
		kiss->pcap_dump = NULL;
	}

	if (fp == NULL) {
		return 0;
	}

	if (fseek(fp, 0, SEEK_END) != 0) {
		return -1;
	}

	if (ftell(fp) != 0) {
		if ((res = kiss_write_pcap_header(fp)) != 0) {
			return res;
		}
	}

	kiss->pcap_dump = fp;

	return 0;
}

static int
kiss_write_pcap_header(FILE *fp)
{
	struct pcap_header h;
	int num;

	h.magic_number = 0xa1b23c4d; /* Nanosecond-resolution type */
	h.version_major = 2;
	h.version_minor = 4;
	h.this_zone = 0;
	h.sigfigs = 0;
	h.snaplen = 65536;
	h.network = LINKTYPE_AX25_KISS;

	num = fwrite(&h, sizeof(h), 1, fp);
	fflush(fp);

	return (num == 1) ? 0 : -1;
}

static int
kiss_pcap_dump(kiss *kiss, struct mbuf *bp)
{
	struct mbuf *tbp;
	struct pcap_packet_header_ns ph;
	struct timeval tv;
	int res;
	size_t size;

	if (kiss->pcap_dump == NULL) {
		return 0;
	}

	if ((res = gettimeofday(&tv, NULL)) != 0) {
		return res;
	}

	size = 0;
	tbp = bp;
	while (tbp != NULL) {
		size += tbp->cnt;
		tbp = tbp->next;
	}

	ph.tm_s = tv.tv_sec;
	ph.tm_ns = tv.tv_usec * 1000;
	ph.capture_length = size;
	ph.packet_length = size;

	if (fwrite(&ph, sizeof(ph), 1, kiss->pcap_dump) != 1) {
		return -1;
	}

	tbp = bp;
	while (tbp != NULL) {
		res = fwrite(tbp->data, sizeof(char), tbp->cnt,
			kiss->pcap_dump);
		if (res != tbp->cnt) {
			return -1;
		}
		tbp = tbp->next;
	}

	if (fflush(kiss->pcap_dump) != 0) {
		return -1;
	}

	return 0;
}

void
kiss_deinit(kiss *kiss)
{
	kiss_set_pcap_dump(kiss, NULL);
	free(kiss);
}
