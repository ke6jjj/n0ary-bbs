#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXINDX	2000

#include "bbslib.h"
#include "vars.h"

FILE *txt, *dat, *idx;
long indx[MAXINDX];

static void usage(const char *prog);

void
move_string(char *str)
{
	int i, j = 8;
	int val = 0;
	char buf[256];

	for(i=0; i<4; i++) {
		if(*str++ == '1')
			val |= j;
		j >>= 1;
	}
	str++;

	sprintf(buf, "%04d ", val);
	if(*str)
		strcat(buf, str);
	else
		strcat(buf, "\n");	
	fputs(buf, dat);
}

int
main(int argc, char *argv[])
{
	int i, maxfound;
	char buf[256], *msgs, *msgdat, *msgidx;

	msgs = "helpmsg.txt";
	msgdat = "helpmsg.dat";
	msgidx = "helpmsg.idx";

	if (argc > 1) {
		if (argc != 4) {
			usage(argv[0]);
			exit(1);
		}
		msgs = argv[1];
		msgdat = argv[2];
		msgidx = argv[3];
	}

	if((txt = fopen(msgs, "r")) == NULL) {
		printf("Failure opening messages.txt\n");
		exit(1);
	}
	if((dat = fopen(msgdat, "w")) == NULL) {
		printf("Failure opening messages.dat\n");
		exit(1);
	}
	if((idx = fopen(msgidx, "w")) == NULL) {
		printf("Failure opening messages.idx\n");
		exit(1);
	}

	for(i=0; i<MAXINDX; i++)
		indx[i] = 0;

	while(fgets(buf,256,txt)) {
		switch(buf[0]) {
		case '!':
			i = atoi(&buf[1]);
			fprintf(dat, "======= %d =======\n", i);
			indx[i] = ftell(dat);
			break;
		case '0':
		case '1':
			move_string(buf);
		}
	}

	maxfound = -1;
	for(i=0; i<MAXINDX; i++) {
		if (indx[i] != 0) {
			maxfound = i;
		}
	}
	
	fwrite(indx, (maxfound+1)*sizeof(int), 1, idx);

	fclose(txt);
	fclose(dat);
	fclose(idx);

	exit(0);
	return 0;
}

static void
usage(const char *prog)
{
	fprintf(stderr, "usage: %s [ <msgtxt> <msgdat> <msgidx> ]\n", prog);
	fprintf(stderr, "Help text compiler.\n");
	fprintf(stderr, "<msgtxt> - Source for help messages. (Default \"messages.txt\")\n");
	fprintf(stderr, "<msgdat> - Name of .dat file to write (Default \"messages.dat\")\n");
	fprintf(stderr, "<msgidx> - Name of .idx file to write (Default \"messages.idx\")\n");
}

