#include <stdlib.h>

#include "c_cmmn.h"
#include "tools.h"
#include "mbuf.h"
#include "kiss.h"
#include "slip.h"
#include "timer.h"
#include "ax25.h"

/* KISS TNC control */
#define	KISS_DATA	0

struct kiss {
	mbuf_recv_fn send;
	void *send_arg;
};

kiss *
kiss_init(void)
{
	kiss *kiss = malloc_struct(kiss);
	if (kiss == NULL)
		return NULL;

	kiss->send = NULL;
	kiss->send_arg = NULL;

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

	return dev->send(dev->send_arg, bp);
}

/* Process incoming KISS TNC frame */
int
kiss_recv(void *devp, struct mbuf *bp)
{
	kiss *dev = (struct kiss *) devp;
	char kisstype;

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

void
kiss_deinit(kiss *kiss)
{
	free(kiss);
}
