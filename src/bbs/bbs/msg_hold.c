#include <stdio.h>
#include <string.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "vars.h"
#include "user.h"
#include "tokens.h"
#include "message.h"
#include "rfc822.h"
#include "function.h"

void
disp_held_reason(char *s)
{
	PRINTF("%s\n", s);
}

int
msg_hold_disp_reason(int num)
{
	char cmd[80];
	sprintf(cmd, "%s %d", msgd_xlate(mWHY), num);
	PRINTF("Msg: %d\n", num);
	msgd_fetch_multi(cmd, disp_held_reason);
	PRINTF("\n");
	return OK;
}

int
msg_hold_release(int num)
{
	char *c = msgd_cmd_num(msgd_xlate(mACTIVE), num);

	if(c[0] == 'O'  && c[1] == 'K')
		return OK;
	return ERROR;
}

int
msg_hold_get_reason(struct msg_dir_entry *m, char *reason)
{
	struct text_line *tl = NULL;
	char buf[80];

	if(reason == NULL) {
		PRINTF("Msg: %5d To: %s@%s Fr: %s Sj: %s\n", m->number,
			m->to.name.str, m->to.at.str, m->from.name.str, m->sub);
		PRINTF("Please enter a brief reason as to why the\n");
		PRINTF("message is being held (/EX, /AB):\n");
		while(TRUE) {
			GETS(buf, 79);
			if(buf[0] == '/') {
				char str[80];
				strcpy(str, buf);
				case_string(str, AllUpperCase);
				if(!strncmp(str, "/EX", 3))
					break;
				if(!strncmp(str, "/AB", 3))
					return OK;
			}
			textline_append(&tl, buf);
		}
		PRINTF("Thank you\n");
	} else
		textline_append(&tl, reason);

	sprintf(buf, "%s %ld", msgd_xlate(mHOLD), m->number);
	msgd_cmd_textline(buf, tl);
	textline_free(tl);
	return OK;
}

int
msg_hold_by_bbs(int num, char *reason)
{
	struct text_line *tl = NULL;
	char buf[80];

	textline_append(&tl, reason);
	sprintf(buf, "%s %d", msgd_xlate(mHOLD), num);
	msgd_cmd_textline(buf, tl);
	textline_free(tl);
	return OK;
}
