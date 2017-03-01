#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/List.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bbstool.h"

Widget usr_parent, usr_list;
void
usr_callback(Widget widget, caddr_t client_data, caddr_t call_data)
{
	static Position x = 225;
	static Position y = 10;
	Arg args[10];
	Cardinal n = 0;

	if(XtIsRealized(usr_parent)) {
		XtSetArg(args[n], XtNx, &x); n++;
		XtSetArg(args[n], XtNy, &y); n++;
		XtGetValues(usr_parent, args, n);
		XtUnrealizeWidget(usr_parent);
		x-=5;
		y-=26;
	} else {
		XtSetArg(args[n], XtNx, x); n++;
		XtSetArg(args[n], XtNy, y); n++;
		XtSetValues(usr_parent, args, n);
		XtRealizeWidget(usr_parent);
	}
}

usr_display()
{
	struct my_active_procs *ap = ActiveProcs;
	XmStringTable str_list;
	Arg args[4];
	Cardinal n = 0;
	int i, j, cnt = 0;

    XmListDeleteAllItems( usr_list);

	while(ap) {
		if(ap->window == USRWIN)
			cnt++;
		NEXT(ap);
	}
	ap = ActiveProcs;

	str_list = (XmStringTable)XtMalloc(cnt * sizeof(XmString *));
	for(i=0,j=0; j<cnt; i++) {
		if(ap->window == USRWIN) {
			build_display(ap);
			str_list[j++] = XmStringCreateSimple(ap->display);
		}
		NEXT(ap);
	}

	XtSetArg(args[n], XmNitems, str_list); n++;
	XtSetArg(args[n], XmNitemCount, cnt); n++;
	XtSetValues(usr_list, args, n);

	for(i=0; i<cnt; i++)
		XmStringFree(str_list[i]);
	XtFree(str_list);
}

void
usr_create_window(void)
{
	XmStringTable str_list;
	int i;
	Arg args[10];
	Cardinal n = 0;
	char title[80];

	sprintf(title, "User List @ %s", Bbs_Host);

    n = 0;
	XtSetArg(args[n], XmNmwmDecorations, Decoration); n++;
	XtSetArg(args[n], XmNtitle, title); n++;
	usr_parent = XtCreateApplicationShell("Users",
		topLevelShellWidgetClass, args, n);

	XtSetArg(args[n], XmNwidth, 600); n++;
	XtSetArg(args[n], XmNheight, 160); n++;
	usr_list = XmCreateScrolledList(usr_parent, "Users", args, n);

	XtAddCallback(usr_list, XmNdefaultActionCallback, proc_connect, NULL);
	XtManageChild(usr_list);
}
