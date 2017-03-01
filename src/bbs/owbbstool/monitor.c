#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrollingL.h>


#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bbstool.h"

void
proc_connect(Widget w, XtPointer client_data, XtPointer cb)
{
#if 0
	char *text, *p, call[10];
	int port;
	XmStringGetLtoR(cb->item, XmSTRING_DEFAULT_CHARSET, &text);
	p = text;
	strcpy(call, get_string(&p));
	port = atoi(&text[31]);
	XtFree(text);

	if(!fork()) {
		char cmd[256];
		sprintf(cmd,
			"/usr/bin/X11/xterm -fn lucidasanstypewriter-10 -name grey -title %s -e telnet %s %d",
			call, BBS_HOST, port);

		system(cmd);
		exit(0);
	}
#endif
}

