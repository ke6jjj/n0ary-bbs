#include <stdio.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "function.h"
#include "bbslib.h"
#include "tokens.h"
#include "process.h"

int
process(void)
{
	FILE *fp;
	char buf[80];

	PRINT("  PID TT STAT  TIME COMMAND\n");
	fp = popen("ps -ax | grep \" bbs \" | grep -v grep", "r");
	while(fgets(buf, 80, fp))
		PRINT(buf);
	pclose(fp);

	return OK;
}

int
uustat(void)
{
	FILE *fp;
	int i, cnt = 0;
	int first_time = TRUE;
	char host[100][20];
	struct TOKEN *t = TokenList;

	NEXT(t);

	if(t->token == END) {
		fp = popen("uuname", "r");
		while(fgets(host[cnt], 80, fp)) {
			char *s = &host[cnt][strlen(host[cnt])-1];
			*s = 0;
			cnt++;
		}
		pclose(fp);
	} else {
		while(t->token != END) {
			case_strcpy(host[cnt], t->lexem, AllLowerCase);
			cnt++;
			NEXT(t);
		}
	}

	for(i=0; i<cnt; i++) {
		char cmd[80];
		char buf[256];
		int msg_cnt = 0;

		sprintf(cmd, "uustat -s%s", host[i]);
		fp = popen(cmd, "r");
		while(fgets(buf, 256, fp))
			msg_cnt++;
		pclose(fp);

		if(msg_cnt) {
			if(first_time) {
				PRINT("Msgs\tMachine\n");
				PRINT("----\t-------\n");
				first_time = FALSE;
			}
			PRINTF("%d\t%s\n", msg_cnt/2, host[i]);
		}	
	}
	if(first_time)
		PRINT("No spooled messages\n");
	return OK;
}

