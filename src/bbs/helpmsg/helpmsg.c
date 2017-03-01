#include <stdio.h>
#include <string.h>
#define MAXINDX	1000

#include "vars.h"

FILE *txt, *dat, *idx;
long indx[MAXINDX];

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

void
main()
{
	int i;
	char buf[256];

	if((txt = fopen("helpmsg.txt", "r")) == NULL) {
		printf("Failure opening messages.txt\n");
		exit(1);
	}
	if((dat = fopen("helpmsg.dat", "w")) == NULL) {
		printf("Failure opening messages.dat\n");
		exit(1);
	}
	if((idx = fopen("helpmsg.idx", "w")) == NULL) {
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
	
#if 0
	for(i=0; i<MAXINDX; i++)
		putw(indx[i], idx);
#else
		/* Fix from Dirk-Jan Koopman */
	fwrite(indx, MAXINDX*sizeof(int), 1, idx);
#endif

	fclose(txt);
	fclose(dat);
	fclose(idx);

	exit(0);
}


