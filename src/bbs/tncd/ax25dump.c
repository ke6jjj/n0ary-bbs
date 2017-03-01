#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "timer.h"
#include "ax25.h"
#include "mbuf.h"
#include "monitor.h"

static int
	build_data_buffer(struct mbuf *bp, u_char **data),
	d_ntohax25(struct ax25 *hdr, unsigned char **str, int *cnt);

static char
	*decode_type(int type);

extern char *Bbs_My_Call;

#ifdef TRACE_DBG
FILE *trfp = stderr;
#else
FILE *trfp = NULL;
#endif

static int
build_data_buffer(struct mbuf *bp, u_char **data)
{
	struct mbuf *tbp = bp;
	int totalsize = 0;
	u_char *p;

	while(tbp != NULLBUF) {
		totalsize += tbp->cnt;
		tbp = tbp->next;
	}

	if((*data = (u_char*)malloc(totalsize)) == NULL)
		return 0;

	tbp = bp;
	p = *data;
	while(tbp != NULLBUF) {
		int i;
		for(i=0; i<tbp->cnt; i++)
			*p++ = tbp->data[i];
		tbp = tbp->next;
	}
	return totalsize;
}

/* Convert a network-format AX.25 header into a host format structure
 * Return -1 if error, number of addresses if OK
 */
static int
d_ntohax25(
	struct ax25 *hdr,	/* Output structure */
	unsigned char **str,
	int *cnt)
{
	register struct ax25_addr *axp;
	char buf[AXALEN];

	if(*cnt < AXALEN)
		return -1;
	memcpy(buf, (char*)*str, AXALEN);
	*str += AXALEN;
	*cnt -= AXALEN;
	getaxaddr(&hdr->dest, buf);

	if(*cnt < AXALEN)
		return -1;
	memcpy(buf, (char*)*str, AXALEN);
	*str += AXALEN;
	*cnt -= AXALEN;
	getaxaddr(&hdr->source, buf);

			/* Process C bits to get command/response indication */

	if((hdr->source.ssid & C) == (hdr->dest.ssid & C))
		hdr->cmdrsp = UNKNOWN;
	else if(hdr->source.ssid & C)
		hdr->cmdrsp = RESPONSE;
	else
		hdr->cmdrsp = COMMAND;

	hdr->ndigis = 0;
	if(hdr->source.ssid & E)
		return 2;	/* No digis */

			/* Process digipeaters */

	for(axp=hdr->digis; axp<&hdr->digis[MAXDIGIS]; axp++) {
		if(*cnt < AXALEN)
			return -1;
		memcpy(buf, (char*)*str, AXALEN);
		*str += AXALEN;
		*cnt -= AXALEN;
		getaxaddr(axp, buf);
		if(axp->ssid & E){	/* Last one */
			hdr->ndigis = axp - hdr->digis + 1;
			return hdr->ndigis + 2;			
		}
	}
	return -1;	/* Too many digis */
}

/* Dump an AX.25 packet header */
void
ax25_dump(struct mbuf *bp)
{
	char tmp[20];
	char control,pid;
	int type;
	struct ax25 hdr;
	struct ax25_addr *hp;
	unsigned char *buf;
	int cnt;
	int me = FALSE;
	char out[4096];

	if(monitor_enabled() == FALSE && trfp == NULL)
		return;

	out[0] = 0;
	cnt = build_data_buffer(bp, &buf);

	/* Extract the address header */
	if(d_ntohax25(&hdr, &buf, &cnt) < 0){
		/* Something wrong with the header */
		strcat(out," bad header!\n");
		if(trfp != NULL) {
			fprintf(trfp, "%s", out);
			fflush(trfp);
		}
		monitor_write(out, me);
		return;
	}
	pax25(tmp, &hdr.source);
	sprintf(out,"%s %s",out, tmp);
	if(!strcmp(tmp, Bbs_My_Call))
		me = TRUE;
	pax25(tmp, &hdr.dest);
	sprintf(out,"%s->%s",out, tmp);
	if(!strcmp(tmp, Bbs_My_Call))
		me = TRUE;
	if(hdr.ndigis > 0){
		strcat(out," v");
		for(hp = &hdr.digis[0]; hp < &hdr.digis[hdr.ndigis]; hp++){
			/* Print digi string */
			pax25(tmp, hp);
			sprintf(out,"%s %s%s",out, tmp,(hp->ssid & REPEATED) ? "*":"");
		}
	}
	if(cnt < 1) {
		strcat(out," no control!\n");
		if(trfp != NULL) {
			fprintf(trfp, "%s", out);
			fflush(trfp);
		}
		monitor_write(out, me);
		return;
	}
	control = *buf++; cnt--;

	type = ftype(control);
	sprintf(out, "%s %s", out, decode_type(type));
	/* Dump poll/final bit */
	if(control & PF){
		switch(hdr.cmdrsp){
		case COMMAND:
			strcat(out, "(P)");
			break;
		case RESPONSE:
			strcat(out,"(F)");
			break;
		default:
			strcat(out,"(P/F)");
			break;
		}
	}
	/* Dump sequence numbers */
	if((type & 0x3) != U)	/* I or S frame? */
		sprintf(out,"%s NR=%d", out, (control>>5)&7);
	if(type == I || type == UI){	
		if(type == I)
			sprintf(out,"%s NS=%d", out, (control>>1)&7);
		/* Decode I field */
		if(cnt > 0) {
			pid = *buf++; cnt--;
			switch(pid & (PID_FIRST | PID_LAST)){
			case PID_FIRST:
				strcat(out," First frag");
				break;
			case PID_LAST:
				strcat(out," Last frag");
				break;
			case PID_FIRST|PID_LAST:
				break;	/* Complete message, say nothing */
			case 0:
				strcat(out," Middle frag");
				break;
			}

			strcat(out," pid=");
			switch(pid & 0x3f){
			case PID_ARP:
				strcat(out,"ARP\n");
				break;
			case PID_NETROM:
				strcat(out,"NET/ROM\n");
				break;
			case PID_IP:
				strcat(out,"IP\n");
				break;
			case PID_NO_L3:
				strcat(out,"Text\n");
				break;
			default:
				sprintf(out,"%s 0x%x\n", out, pid);
			}

			sprintf(out, "%s cnt:%d\n", out, cnt);
			while(cnt > 0) {
				int i;

				strcat(out, "\t");
				for(i=0; i<65; i++) {
					if((*buf >= ' ') && (*buf <= '~'))
						sprintf(out, "%s%c", out, *buf);
					else {
						sprintf(out, "%s<%02x>", out, (unsigned char)*buf);
						i += 3;
					}
					buf++;
					if(--cnt == 0)
						break;
				}
				strcat(out, "\n");
			}
				
		} else
			strcat(out, "\n");

	} else 
		if(type == FRMR /* && cnt > 2 */){
			memcpy(tmp, (char*)buf, 3); buf+=3; cnt-=3;
			sprintf(out, "%s: %s", out, decode_type(ftype(tmp[0])));
			sprintf(out, "%s Vr = %d Vs = %d", out, (tmp[1] >> 5) & MMASK,
				(tmp[1] >> 1) & MMASK);
			if(tmp[2] & W)
				strcat(out," Invalid control field");
			if(tmp[2] & X)
				strcat(out," Illegal I-field");
			if(tmp[2] & Y)
				strcat(out," Too-long I-field");
			if(tmp[2] & Z)
				strcat(out," Invalid seq number");
			strcat(out, "\n");
		} else
			strcat(out, "\n");

	if(trfp != NULL) {
		fprintf(trfp, "%s", out);
		fflush(trfp);
	}
	monitor_write(out, me);
}
/* Display NET/ROM network and transport headers */
static char *
decode_type(int type)
{
	switch((u_char)(type)){
	case I:
		return "I";
	case SABM:
		return "SABM";
	case DISC:
		return "DISC";
	case DM:
		return "DM";
	case UA:
		return "UA";
	case RR:
		return "RR";
	case RNR:
		return "RNR";
	case REJ:
		return "REJ";
	case FRMR:
		return "FRMR";
	case UI:
		return "UI";
	default:
		return "[invalid]";
	}
}

