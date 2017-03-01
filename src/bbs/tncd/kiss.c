#include "c_cmmn.h"
#if 0
#include "global.h"
#include "trace.h"
#endif
#include "mbuf.h"
#include "kiss.h"
#include "slip.h"
#include "timer.h"
#include "ax25.h"

/* Send raw data packet on KISS TNC */
int
kiss_raw(int dev, struct mbuf *data)
{
	struct mbuf *bp;

	ax25_dump(data);

	/* Put type field for KISS TNC on front */
	if((bp = pushdown(data,1)) == NULLBUF){
		free_p(data);
		return 0;
	}
	bp->data[0] = KISS_DATA;

	return slip_raw(dev, bp);
}

/* Process incoming KISS TNC frame */
void
kiss_recv(int dev, struct mbuf *bp)
{
	char kisstype;

	kisstype = pullchar(&bp);
	ax25_dump(bp);
	switch(kisstype & 0xf){
	case KISS_DATA:
		ax_recv(dev, bp);
		break;
	}
}
