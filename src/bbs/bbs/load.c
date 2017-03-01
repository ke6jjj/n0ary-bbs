#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "bbslib.h"
#include "tokens.h"
#include "vars.h"
#include "function.h"

int
load_t(void)
{
	struct TOKEN *t = TokenList;
	int num = 0;
	char buf[256];
	FILE *fp;

	NEXT(t);

	switch(t->token) {
	case TNC144:
		num = 0; break;
	case TNC220:
		num = 1; break;
	case TNC440:
		num = 2; break;
	case CONSOLE:
		num = 5; break;
	case PHONE:
		num = 3; break;
	}

	sprintf(buf, "%s/time%d", Bbs_History_Path, num);
	if((fp = fopen(buf, "r")) == NULL)
		return ERROR;

	while(fgets(buf, 256, fp)) {
		PRINT(buf);
	}
	fclose(fp);

	return OK;
}
