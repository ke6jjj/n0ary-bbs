#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/BulletinB.h>
#include <Xm/RowColumn.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bbstool.h"

#define MAXPORTS 30

Widget lck_parent, lck[MAXPORTS], clr_all[3], lck_all[3];
Widget lock_all_ports, clear_all_ports;

void
all_ports(Widget widget, caddr_t client_data, caddr_t call_data)
{
	int i, func = (widget == lock_all_ports) ? TRUE:FALSE;
	struct PortDefinition *pd = port_table();

	while(pd) {
		lck_toggle(TRUE, pd->name, func);
		NEXT(pd);
	}
}

void
single_port(Widget widget, caddr_t client_data, caddr_t call_data)
{
	int i = 0, func = (XmToggleButtonGetState(widget)) ? FALSE:TRUE;
	struct PortDefinition *pd = port_table();

	while(pd) {
		if(lck[i] == widget) {
			lck_toggle(TRUE, pd->name, func);
			return;
		}
		NEXT(pd);
		i++;
	}
}

void
lck_ports(Widget widget, caddr_t client_data, caddr_t call_data)
{
	int i;
	struct PortDefinition *pd = port_table();

	for(i=0; i<3; i++)
		if(widget == lck_all[i])
			break;

	while(pd) {
		switch(i) {
		case 0:
			if(pd->type == tTNC)
				lck_toggle(TRUE, pd->name, TRUE);
			break;
		case 1:
			if(pd->type == tPHONE)
				lck_toggle(TRUE, pd->name, TRUE);
			break;
		default:
			if(pd->type != tPHONE && pd->type != tTNC)
				lck_toggle(TRUE, pd->name, TRUE);
			break;
		}
		NEXT(pd);
	}

	XmToggleButtonSetState(widget, FALSE, FALSE);
}

void
clr_ports(Widget widget, caddr_t client_data, caddr_t call_data)
{
	int i;
	struct PortDefinition *pd = port_table();

	for(i=0; i<3; i++)
		if(widget == clr_all[i])
			break;

	while(pd) {
		switch(i) {
		case 0:
			if(pd->type == tTNC)
				lck_toggle(TRUE, pd->name, FALSE);
			break;
		case 1:
			if(pd->type == tPHONE)
				lck_toggle(TRUE, pd->name, FALSE);
			break;
		default:
			if(pd->type != tPHONE && pd->type != tTNC)
				lck_toggle(TRUE, pd->name, FALSE);
			break;
		}
	}
}

void
lck_callback(Widget widget, caddr_t client_data, caddr_t call_data)
{
	static Position x = 650;
	static Position y = 90;
	Arg args[10];
	Cardinal n = 0;

	if(XtIsRealized(lck_parent)) {
		XtSetArg(args[n], XtNx, &x); n++;
		XtSetArg(args[n], XtNy, &y); n++;
		XtGetValues(lck_parent, args, n);
		XtUnrealizeWidget(lck_parent);
		x-=5;
		y-=26;
	} else {
		XtSetArg(args[n], XtNx, x); n++;
		XtSetArg(args[n], XtNy, y); n++;
		XtSetValues(lck_parent, args, n);
		XtRealizeWidget(lck_parent);
	}
}

lck_toggle(int issue, char *name, int state)
{
	XmToggleButtonSetState(lck[port_indx(name)], state, FALSE);
	if(issue) {
		if(state == FALSE)
			bbsd_unlock(name);
		else
			bbsd_lock(name, "BBS maintenance");
	}
}

void
lck_create_window(void)
{
	Widget lck_bb, lck_rowcol[3];
	Arg args[10];
	Cardinal n;
	int i;
	int loc = 20;
	XmString str[3];
	struct PortDefinition *pd = port_table();
	char title[80];

	sprintf(title, "Port Locks @ %s", Bbs_Host);

    n = 0;
	XtSetArg(args[n], XmNmwmDecorations, Decoration); n++;
	XtSetArg(args[n], XmNtitle, title); n++;
	lck_parent = XtCreateApplicationShell("PortLocks",
		topLevelShellWidgetClass, args, n);

	lck_bb = XtVaCreateManagedWidget("PortLocks",
		xmBulletinBoardWidgetClass, lck_parent,
		XtNwidth, 300,
		XtNheight, 200,
		NULL, 0);

	
	str[0] = XmStringCreateSimple("  TNC");
	str[1] = XmStringCreateSimple(" Phone");
	str[2] = XmStringCreateSimple(" Other");

	for(i=0; i<3; i++) {
		XtVaCreateManagedWidget("PortText", xmLabelWidgetClass, lck_bb,
			XmNlabelString, str[i],
			XmNx, loc+15,
			XmNy, 0,
			NULL);
		XmStringFree(str[i]);

		lck_all[i] = XtVaCreateManagedWidget("LCK",
			xmPushButtonWidgetClass, lck_bb,
			XmNx, loc+10,
			XmNy, 28,
			NULL, 0);
		XtAddCallback(lck_all[i], XmNarmCallback, lck_ports, NULL);
		clr_all[i] = XtVaCreateManagedWidget("CLR",
			xmPushButtonWidgetClass, lck_bb,
			XmNx, loc+38,
			XmNy, 28,
			NULL, 0);
		XtAddCallback(clr_all[i], XmNarmCallback, clr_ports, NULL);

		lck_rowcol[i] = XtVaCreateManagedWidget("LockTNC",
			xmRowColumnWidgetClass, lck_bb,
			XtNx, loc,
			XtNy, 50,
			NULL, 0);

		loc+=90;
	}

	i = 0;
	while(pd) {
		char buf[80];
		int column;
		char *p = pd->alias;
		NextChar(p);
		strcpy(buf, p);
		strcat(buf, "      ");
		buf[8] = 0;

		switch(pd->type) {
		case tTNC:
			column = 0; break;
		case tPHONE:
			column = 1; break;
		default:
			column = 2; break;
		}

		lck[i] = XtVaCreateManagedWidget(buf,
			xmToggleButtonWidgetClass, lck_rowcol[column], NULL);
		XtAddCallback(lck[i], XmNarmCallback, single_port, NULL);
		NEXT(pd);
		i++;
	}

	lock_all_ports = XtVaCreateManagedWidget("LOCK ALL",
		xmPushButtonWidgetClass, lck_bb,
		XmNwidth, 80,
		XmNx, 110,
		XmNy, 135,
		NULL, 0);

	clear_all_ports = XtVaCreateManagedWidget("CLEAR ALL",
		xmPushButtonWidgetClass, lck_bb,
		XmNwidth, 80,
		XmNx, 110,
		XmNy, 160,
		NULL, 0);

	XtAddCallback(lock_all_ports, XmNarmCallback, all_ports, NULL);
	XtAddCallback(clear_all_ports, XmNarmCallback, all_ports, NULL);
}
