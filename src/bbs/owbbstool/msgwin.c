#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/PushBG.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bbstool.h"

static struct text_line *msg_tl;

Widget msg_parent, msg_list;
Widget group_parent, group_list, grouptext;
Widget route_parent, route_list;
Widget read_parent, read_rfc, read_body, read_header;

char *sample = {"None"};

void
connect_to_msgd(void)
{
	while(msgd_open())
		sleep(10);
	msgd_cmd_string(msgd_xlate(mUSER), Bbs_Call);
	msgd_cmd(msgd_xlate(mSYSOP));
	msgd_cmd_string(msgd_xlate(mDISP), "VERBOSE");
}

msg_callback(Widget widget, caddr_t client_data, caddr_t call_data)
{
	static Position x = 240;
	static Position y = 215;
	Arg args[10];
	Cardinal n = 0;

	if(XtIsRealized(msg_parent)) {
		msgd_close();
		XtSetArg(args[n], XtNx, &x); n++;
		XtSetArg(args[n], XtNy, &y); n++;
		XtGetValues(msg_parent, args, n);
		XtUnrealizeWidget(msg_parent);
		x-=5;
		y-=26;
	} else {
		connect_to_msgd();
		XtSetArg(args[n], XtNx, x); n++;
		XtSetArg(args[n], XtNy, y); n++;
		XtSetValues(msg_parent, args, n);
		XtRealizeWidget(msg_parent);
	}
}

void
msg_choose(Widget w, caddr_t client_data, XmListCallbackStruct *cb)
{
	struct text_line *tl = NULL;
	struct msg_dir_entry msg;
	char buf[1024];
	char *text, *p;
	static XmTextPosition pos = 0;
	int in_header = TRUE;
	Arg args[10];
	Cardinal n = 0;
	static Position x = 530;
	static Position y = 290;

	XmStringGetLtoR(cb->item, XmSTRING_DEFAULT_CHARSET, &text);

	bzero(&msg, sizeof(msg));
	p = text;
	msg.number = get_number(&p);
	XtFree(text);

	msg_ReadRfc(&msg);
	buf[0] = 0;
	XmTextSetString(read_rfc, buf);

	tl = msg.header;
	while(tl) {
		if(strlen(tl->s) > 1024)
			tl->s[1023] = 0;
		sprintf(buf, "%s\n", tl->s);
		XmTextInsert(read_rfc, pos, buf);
		pos += strlen(buf);
		NEXT(tl);
	}

	pos = 0;
	XtVaSetValues(read_rfc, XmNcursorPosition, pos, NULL);
	XmTextShowPosition(read_rfc, pos);

	msg_ReadBody(&msg);
	buf[0] = 0;
	XmTextSetString(read_body, buf);

	tl = msg.body;
	while(tl) {
		if(strlen(tl->s) > 1024)
			tl->s[1023] = 0;
		if(in_header) {
			if(strncmp(tl->s, "R:", 2)) {
				in_header = FALSE;
				pos = 0;
			} else {
				sprintf(buf, "%s\n", tl->s);
				XmTextInsert(read_header, pos, buf);
				pos += strlen(buf);
			}
		} else {
			sprintf(buf, "%s\n", tl->s);
			XmTextInsert(read_body, pos, buf);
			pos += strlen(buf);
		}
		NEXT(tl);
	}

	pos = 0;
	XtVaSetValues(read_header, XmNcursorPosition, pos, NULL);
	XmTextShowPosition(read_header, pos);
	XtVaSetValues(read_body, XmNcursorPosition, pos, NULL);
	XmTextShowPosition(read_body, pos);

	n = 0;
	XtSetArg(args[n], XtNx, &x); n++;
	XtSetArg(args[n], XtNy, &y); n++;
	XtGetValues(read_parent, args, n);
	XtRealizeWidget(read_parent);
}

void
rte_choose(Widget w, caddr_t client_data, XmListCallbackStruct *cb)
{
#if 0
	char *text, *p, group[10];
	XmStringGetLtoR(cb->item, XmSTRING_DEFAULT_CHARSET, &text);
	p = text;
	strcpy(group, get_string(&p));
	XtFree(text);

	XmTextFieldSetString(grouptext, "Please wait....");

	msg_tl = textline_free(msg_tl);
	msgd_cmd_string(msgd_xlate(mGROUP), group);
	msgd_cmd(msgd_xlate(mFLUSH));

	msg_display();
	XmTextFieldSetString(grouptext, group);
#endif
}

void
grp_choose(Widget w, caddr_t client_data, XmListCallbackStruct *cb)
{
	char *text, *p, group[10];
	XmStringGetLtoR(cb->item, XmSTRING_DEFAULT_CHARSET, &text);
	p = text;
	strcpy(group, get_string(&p));
	XtFree(text);

	XmTextFieldSetString(grouptext, "Please wait....");

	msg_tl = textline_free(msg_tl);
	msgd_cmd_string(msgd_xlate(mGROUP), group);
	msgd_cmd(msgd_xlate(mFLUSH));

	msg_display();
	XmTextFieldSetString(grouptext, group);
}

void
read_done_callback(Widget w, caddr_t client_data, 
					XmPushButtonCallbackStruct call_value)
{
	static Position x = 530;
	static Position y = 290;
	Arg args[10];
	Cardinal n = 0;

	if(XtIsRealized(read_parent)) {
		XtSetArg(args[n], XtNx, &x); n++;
		XtSetArg(args[n], XtNy, &y); n++;
		XtGetValues(read_parent, args, n);
		XtUnrealizeWidget(read_parent);
		x-=5;
		y-=26;
	}
}

void
route_list_callback(Widget w, caddr_t client_data, 
					XmPushButtonCallbackStruct call_value)
{
	static Position x = 890;
	static Position y = 60;
	Arg args[10];
	Cardinal n = 0;

	if(XtIsRealized(route_parent)) {
		XtSetArg(args[n], XtNx, &x); n++;
		XtSetArg(args[n], XtNy, &y); n++;
		XtGetValues(route_parent, args, n);
		XtUnrealizeWidget(route_parent);
		x-=5;
		y-=26;
	} else {
		rte_display();
		XtSetArg(args[n], XtNx, x); n++;
		XtSetArg(args[n], XtNy, y); n++;
		XtSetValues(route_parent, args, n);
		XtRealizeWidget(route_parent);
	}
}

void
group_list_callback(Widget w, caddr_t client_data, 
					XmPushButtonCallbackStruct call_value)
{
	static Position x = 90;
	static Position y = 250;
	Arg args[10];
	Cardinal n = 0;

	if(XtIsRealized(group_parent)) {
		XtSetArg(args[n], XtNx, &x); n++;
		XtSetArg(args[n], XtNy, &y); n++;
		XtGetValues(group_parent, args, n);
		XtUnrealizeWidget(group_parent);
		x-=5;
		y-=26;
	} else {
		grp_display();
		XtSetArg(args[n], XtNx, x); n++;
		XtSetArg(args[n], XtNy, y); n++;
		XtSetValues(group_parent, args, n);
		XtRealizeWidget(group_parent);
	}
}

rte_display(void)
{
	struct text_line *tl, *rte_tl = NULL;
	XmStringTable str_list;
	Arg args[4];
	Cardinal n = 0;
	int i = 0;
	int route_cnt = 0;

	textline_append(&rte_tl, "Fire");
	textline_append(&rte_tl, "Police");
	textline_append(&rte_tl, "Public Safety");
	textline_append(&rte_tl, "Forestry");
	textline_append(&rte_tl, "Damage Assessment");
	textline_append(&rte_tl, "Red Cross");
	textline_append(&rte_tl, "EOC");

	tl = rte_tl;
	while(tl) {
		route_cnt++;
		NEXT(tl);
	}

	str_list = (XmStringTable)XtMalloc(route_cnt * sizeof(XmString *));

	tl = rte_tl;
	while(tl) {
		str_list[i++] = XmStringCreateSimple(tl->s);
		NEXT(tl);
	}

	rte_tl = textline_free(rte_tl);

	XtSetArg(args[n], XmNitems, str_list); n++;
	XtSetArg(args[n], XmNitemCount, route_cnt); n++;
	XtSetValues(route_list, args, n);

	for(i=0; i<route_cnt; i++)
		XmStringFree(str_list[i]);
	XtFree(str_list);
}

grp_display(void)
{
	struct text_line *tl, *grp_tl = NULL;
	XmStringTable str_list;
	Arg args[4];
	Cardinal n = 0;
	int i = 0;
	int group_cnt = 0;

	msgd_fetch_textline(msgd_xlate(mGROUP), &grp_tl);

	tl = grp_tl;
	while(tl) {
		group_cnt++;
		NEXT(tl);
	}

	str_list = (XmStringTable)XtMalloc(group_cnt * sizeof(XmString *));

	tl = grp_tl;
	while(tl) {
		char buf[80], *p = tl->s;
		char *grp = get_string(&p);
		int num = get_number(&p);

		sprintf(buf, "%-10s  %d", grp, num);
		str_list[i++] = XmStringCreateSimple(buf);
		NEXT(tl);
	}

	grp_tl = textline_free(grp_tl);

	XtSetArg(args[n], XmNitems, str_list); n++;
	XtSetArg(args[n], XmNitemCount, group_cnt); n++;
	XtSetValues(group_list, args, n);

	for(i=0; i<group_cnt; i++)
		XmStringFree(str_list[i]);
	XtFree(str_list);
}

msg_display(void)
{
	struct text_line *tl;
	XmStringTable str_list;
	Arg args[4];
	Cardinal n = 0;
	int i = 0;
	int msg_cnt = 0;

	msg_tl = textline_free(msg_tl);
	msgd_fetch_textline(msgd_xlate(mLIST), &msg_tl);

	tl = msg_tl;
	while(tl) {
		msg_cnt++;
		NEXT(tl);
	}

	str_list = (XmStringTable)XtMalloc(msg_cnt * sizeof(XmString *));

	tl = msg_tl;
	while(tl) {
		str_list[i++] = XmStringCreateSimple(tl->s);
		NEXT(tl);
	}

	XtSetArg(args[n], XmNitems, str_list); n++;
	XtSetArg(args[n], XmNitemCount, msg_cnt); n++;
	XtSetValues(msg_list, args, n);

	for(i=0; i<msg_cnt; i++)
		XmStringFree(str_list[i]);
	XtFree(str_list);
}

msg_read_create_window(void)
{
	Arg args[10];
	Cardinal n = 0;
	Widget read_rc, read_button_rc, read_done_pb, read_route_pb;

	read_rc = XtVaCreateWidget("read_rc", xmRowColumnWidgetClass, read_parent,
		NULL);
	XtManageChild(read_rc);

	read_button_rc =
		XtVaCreateWidget("read_button_rc", xmRowColumnWidgetClass, read_rc,
			XmNorientation, XmHORIZONTAL,
			NULL);
	XtManageChild(read_button_rc);

	n = 0;
	read_done_pb =
		XtCreateWidget("Done", xmPushButtonGadgetClass, read_button_rc, args, n);
	XtAddCallback(read_done_pb, XmNdisarmCallback, read_done_callback, NULL);
	XtManageChild(read_done_pb);

	n = 0;
	read_route_pb =
		XtCreateWidget("Route", xmPushButtonGadgetClass, read_button_rc, args, n);
	XtAddCallback(read_route_pb, XmNdisarmCallback, route_list_callback, NULL);
	XtManageChild(read_route_pb);

	n = 0;
	XtSetArg(args[n], XmNrows, 6); n++;
	XtSetArg(args[n], XmNcolumns, 80); n++;
	XtSetArg(args[n], XmNeditable, False); n++;
	XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
	XtSetArg(args[n], XmNwordWrap, True); n++;
	XtSetArg(args[n], XmNscrollVertical, True); n++;
	XtSetArg(args[n], XmNscrollHorizontal, False); n++;
	XtSetArg(args[n], XmNcursorPositionVisible, False); n++;
	read_rfc = XmCreateScrolledText(read_rc, "read_rfc", args, n);
	XtManageChild(read_rfc);

	n = 0;
	XtSetArg(args[n], XmNrows, 4); n++;
	XtSetArg(args[n], XmNcolumns, 80); n++;
	XtSetArg(args[n], XmNeditable, False); n++;
	XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
	XtSetArg(args[n], XmNwordWrap, True); n++;
	XtSetArg(args[n], XmNscrollVertical, True); n++;
	XtSetArg(args[n], XmNscrollHorizontal, False); n++;
	XtSetArg(args[n], XmNcursorPositionVisible, False); n++;
	read_header = XmCreateScrolledText(read_rc, "read_header", args, n);
	XtManageChild(read_header);

	n = 0;
	XtSetArg(args[n], XmNrows, 24); n++;
	XtSetArg(args[n], XmNcolumns, 80); n++;
	XtSetArg(args[n], XmNeditable, False); n++;
	XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
	XtSetArg(args[n], XmNwordWrap, True); n++;
	XtSetArg(args[n], XmNscrollVertical, True); n++;
	XtSetArg(args[n], XmNscrollHorizontal, False); n++;
	XtSetArg(args[n], XmNcursorPositionVisible, False); n++;
	read_body = XmCreateScrolledText(read_rc, "read_body", args, n);
	XtManageChild(read_body);
}

msg_route_create_window(void)
{
	Arg args[10];
	Cardinal n = 0;
	Widget rte_rc, rte_pb;

	rte_rc = XtVaCreateWidget("rte_rc", xmRowColumnWidgetClass, route_parent,
		NULL);
	XtManageChild(rte_rc);

	n = 0;
	XtSetArg(args[n], XmNheight, 160); n++;
	route_list = XmCreateScrolledList(rte_rc, "route_list", args, n);

	XtAddCallback(route_list, XmNdefaultActionCallback, rte_choose, NULL);
	XtManageChild(route_list);

	n = 0;
	rte_pb = XtCreateWidget("Done", xmPushButtonGadgetClass, rte_rc, args, n);

	XtAddCallback(rte_pb, XmNdisarmCallback, route_list_callback, NULL);
	XtManageChild(rte_pb);
}

msg_group_create_window(void)
{
	Arg args[10];
	Cardinal n = 0;
	Widget grp_rc, grp_pb;

	grp_rc = XtVaCreateWidget("grp_rc", xmRowColumnWidgetClass, group_parent,
		NULL);
	XtManageChild(grp_rc);

	n = 0;
	XtSetArg(args[n], XmNheight, 250); n++;
	group_list = XmCreateScrolledList(grp_rc, "group_list", args, n);

	XtAddCallback(group_list, XmNdefaultActionCallback, grp_choose, NULL);
	XtManageChild(group_list);

	n = 0;
	grp_pb = XtCreateWidget("Done", xmPushButtonGadgetClass, grp_rc, args, n);

	XtAddCallback(grp_pb, XmNdisarmCallback, group_list_callback, NULL);
	XtManageChild(grp_pb);
}

void
msg_create_window(void)
{
	XmStringTable str_list;
	int i;
	Arg args[10];
	Cardinal n = 0;

	Widget rc0, grouppb, glform, routepb;

	n = 0;
	XtSetArg(args[n], XmNmwmDecorations, Decoration); n++;
	XtSetArg(args[n], XmNtitle, "Messages List"); n++;
	msg_parent = XtCreateApplicationShell("Messages",
		topLevelShellWidgetClass, args, n);

	n = 0;
	XtSetArg(args[n], XmNmwmDecorations, Decoration); n++;
	XtSetArg(args[n], XmNtitle, "Group List"); n++;
	group_parent = XtCreateApplicationShell("GroupList",
		topLevelShellWidgetClass, args, n);

	n = 0;
	XtSetArg(args[n], XmNmwmDecorations, Decoration); n++;
	XtSetArg(args[n], XmNtitle, "Route List"); n++;
	route_parent = XtCreateApplicationShell("RouteList",
		topLevelShellWidgetClass, args, n);

	n = 0;
	XtSetArg(args[n], XmNmwmDecorations, Decoration); n++;
	XtSetArg(args[n], XmNtitle, "Message"); n++;
	read_parent = XtCreateApplicationShell("MessageRead",
		topLevelShellWidgetClass, args, n);

	rc0 = XtVaCreateWidget( "rc0", xmRowColumnWidgetClass, msg_parent, NULL);
	XtManageChild(rc0);
	
	glform = XtCreateWidget("glform", xmFormWidgetClass, rc0, NULL, 0);
	XtManageChild(glform);

	n = 0;
	XtSetArg( args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg( args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	grouppb = XtCreateWidget("Group:", xmPushButtonGadgetClass,
								glform, args, n);
	XtAddCallback(grouppb, XmNdisarmCallback, group_list_callback, NULL);
	XtManageChild(grouppb);

	n = 0;
	XtSetArg( args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg( args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg( args[n], XmNleftWidget, grouppb); n++;
	XtSetArg( args[n], XmNeditable, False); n++;
	XtSetArg( args[n], XmNcursorPositionVisible, False); n++;
	XtSetArg( args[n], XmNcolumns, 70); n++;
	grouptext = XtCreateWidget("grouptext", xmTextFieldWidgetClass,
								glform, args, n);
	XtManageChild(grouptext);

	n = 0;
	XtSetArg( args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg( args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg( args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
	XtSetArg( args[n], XmNleftWidget, grouptext); n++;
	routepb = XtCreateWidget("Routing", xmPushButtonGadgetClass,
								glform, args, n);
	XtAddCallback(routepb, XmNdisarmCallback, route_list_callback, NULL);
	XtManageChild(routepb);

	XtSetArg(args[n], XmNheight, 400); n++;
	msg_list = XmCreateScrolledList(rc0, "msg_list", args, n);
	XtAddCallback(msg_list, XmNdefaultActionCallback, msg_choose, NULL);
	XtManageChild(msg_list);

	msg_group_create_window();
	msg_read_create_window();
	msg_route_create_window();

	XmTextFieldSetString(grouptext, sample);
}

