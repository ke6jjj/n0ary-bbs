#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrollingL.h>
#include <Xol/ControlAre.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bbstool.h"

OlListToken (*ulAddItem)();
void	(*ulDeleteItem)();
void	(*ulTouchItem)();
void	(*ulUpdateView)();

Widget usr_parent, usr_list;
void
usr_callback(Widget widget, caddr_t client_data, caddr_t call_data)
{
	static Position x = 225;
	static Position y = 10;
	Arg args[10];
	int n = 0;

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

usr_delete(struct active_users *ap)
{
	ulDeleteItem(usr_list, ap->token);
}

usr_add(struct active_users *ap, OlListToken token)
{
	OlListItem item;

	ulUpdateView(usr_list, FALSE);
	item.label_type = OL_STRING;
	item.attr = 2;
	item.label = ap->label = XtNewString(ap->display);
	item.mnemonic = NULL;
	ap->token = ulAddItem(usr_list, 0, token, item);
	ulUpdateView(usr_list, TRUE);
}

#if 0
usr_display()
{
	struct active_users *ap = ActiveUsers;
	Arg args[10];
	int n = 0;
	int i, j, cnt = 0;

	while(ap) {
		if(ap->window == USRWIN)
			cnt++;
		NEXT(ap);
	}
	ap = ActiveUsers;

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
#endif

	
void
usr_create_window(void)
{
	int i;
	Arg args[10];
	int n = 0;

	usr_parent = XtCreateApplicationShell("Users",
		applicationShellWidgetClass, NULL, 0);

	XtSetArg(args[n], XtNviewHeight, 10); n++;
	XtSetArg(args[n], XtNwidth, 900); n++;
	usr_list = XtCreateManagedWidget("Users", 
		scrollingListWidgetClass, usr_parent, args, n);

	n = 0;
	XtSetArg(args[n], XtNapplAddItem, &ulAddItem); n++;
	XtSetArg(args[n], XtNapplDeleteItem, &ulDeleteItem); n++;
	XtSetArg(args[n], XtNapplTouchItem, &ulTouchItem); n++;
	XtSetArg(args[n], XtNapplUpdateView, &ulUpdateView); n++;
	XtGetValues(usr_list, args, n);

	XtAddCallback(usr_list, XtNuserMakeCurrent, proc_connect, NULL);
	XtManageChild(usr_list);
}
