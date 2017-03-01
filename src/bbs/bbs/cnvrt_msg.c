#include <stdio.h>
#include <string.h>
#include "c_cmmn.h"
#include "common.h"


#define MAXUSERS_FIELDS	20

struct msg_address {
	struct call_and_checksum
		name, at;
	char address[LenHLOC];
};

struct old_message_directory_entry {
	long	number;
	long	size;
	long	flags;

	char	bid[LenBID];
	char	sub[60];
	char	passwd[20];

	struct msg_address to, from;

	long	fwd_mask;

	time_t  edate;

	time_t	cdate;
	time_t	odate;
	time_t	kdate;

	time_t	ndate;
	char	deliverer[10];
	char	padding[2];

	long	users[MAXUSERS_FIELDS];
	long	read_cnt;

	char	spare[16];
} omsg;

#define MAX_READ_BY		100

struct new_message_directory_entry {
	long	number;
	long	size;
	long	flags;

	char	bid[LenBID];
	char	sub[60];
	char	passwd[20];

	struct msg_address to, from;

	long	fwd_mask;

	time_t  edate;

	time_t	cdate;
	time_t	odate;
	time_t	kdate;

	time_t	ndate;
	char	deliverer[10];
	char	padding[2];

	short	read_by[MAX_READ_BY];
	long	read_cnt;

	char	spare[16];
} nmsg;


main()
{
	int i, j, unum, mask;
	FILE *fpr = fopen("/bbs/message/MsgDir", "r");
	FILE *fpw = fopen("/bbs/message/MsgDir.new", "w");

	while(fread(&omsg, sizeof(omsg), 1, fpr)) {
		
		bzero(&nmsg, sizeof(nmsg));

		nmsg.number = omsg.number;
		nmsg.size = omsg.size;
		nmsg.flags = omsg.flags;

		strncpy(nmsg.bid, omsg.bid, LenBID);
		strncpy(nmsg.sub, omsg.sub, 60);
		strncpy(nmsg.passwd, omsg.passwd, 20);

		nmsg.to.name.sum = omsg.to.name.sum;
		strncpy(nmsg.to.name.str, omsg.to.name.str, LenCALL);
		nmsg.to.at.sum = omsg.to.at.sum;
		strncpy(nmsg.to.at.str, omsg.to.at.str, LenCALL);
		if(omsg.to.address[0] == '.')
			strncpy(nmsg.to.address, &omsg.to.address[1], LenHLOC);
		else
			strncpy(nmsg.to.address, omsg.to.address, LenHLOC);

		nmsg.from.name.sum = omsg.from.name.sum;
		strncpy(nmsg.from.name.str, omsg.from.name.str, LenCALL);
		nmsg.from.at.sum = omsg.from.at.sum;
		strncpy(nmsg.from.at.str, omsg.from.at.str, LenCALL);
		strncpy(nmsg.from.address, omsg.from.address, LenHLOC);

		nmsg.fwd_mask = omsg.fwd_mask;
		nmsg.edate = omsg.edate;
		nmsg.cdate = omsg.cdate;
		nmsg.odate = omsg.odate;
		nmsg.kdate = omsg.kdate;
		nmsg.ndate = omsg.ndate;

		strncpy(nmsg.deliverer, omsg.deliverer, 10);

		for(i=0, unum=0; i<MAXUSERS_FIELDS; i++) {
			for(j=0, mask=1; j<32; j++, mask<<=1) {
				if(omsg.users[i] & mask)
					nmsg.read_by[nmsg.read_cnt++] = (short)unum;
				unum++;

				if(nmsg.read_cnt == MAX_READ_BY)
					break;
			}
			if(nmsg.read_cnt == MAX_READ_BY)
				break;
		}

		fwrite(&nmsg, sizeof(nmsg), 1, fpw);
	}

	fclose(fpr);
	fclose(fpw);
}
