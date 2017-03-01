#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrollingL.h>
#include <Xol/ControlAre.h>
#include <Xol/CheckBox.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bbstool.h"

#define BUTTONCNT	5

char
	*Bbs_Call;

struct active_users *ActiveUsers = NULL;

static char *button_txt[BUTTONCNT] = {
	"UserList", "MessageList", "DaemonList", "PortLocks", "Quit"
};

struct ConfigurationList ConfigList[] = {
	{ "These entries appear in all daemons", tCOMMENT, NULL},
	{ "BBS_HOST",           tSTRING,    (int*)&Bbs_Host},
	{ "BBSD_PORT",          tINT,       (int*)&Bbsd_Port },
	{ "", 	              tCOMMENT,   NULL},
	{ "BBS_CALL",           tSTRING,    (int*)&Bbs_Call},
	{ NULL,                 tEND, NULL}};

#if 0
extern void msg_callback();
#endif

void
quit_callback(Widget widget, caddr_t client_data, caddr_t call_data)
{
	Display *display;
	display = XtDisplay(widget);
	XtCloseDisplay(display);
	exit(0);
}

build_display(struct active_users *ap) 
{
	int hours, minutes, seconds, delta;
	char howlong[80];
	char idle[80];
	long t1 = time(NULL);

	delta = t1 -  ap->ctime;
	if(delta < (10 * tHour)) {
		hours = delta/3600;
		delta %= 3600;
		minutes = delta/60;
		seconds = delta%60;

		sprintf(howlong, "%2d:%02d:%02d", hours, minutes, seconds);
	} else
		if(delta < (3 * tDay))
			sprintf(howlong, " %02d hrs ", delta / tHour);
		else
			if(delta < (3 * tWeek))
				sprintf(howlong, " %02d days", delta / tDay);
			else
				sprintf(howlong, " %02d wks ", delta / tWeek);
			
	delta = t1 -  ap->idle;
	if(delta < (10 * tHour)) {
		hours = delta/3600;
		delta %= 3600;
		minutes = delta/60;
		seconds = delta%60;

		sprintf(idle, "%2d:%02d:%02d", hours, minutes, seconds);
	} else
		if(delta < (3 * tDay))
			sprintf(idle, " %02d hrs ", delta / tHour);
		else
			if(delta < (3 * tWeek))
				sprintf(idle, " %02d days", delta / tDay);
			else
				sprintf(idle, " %02d wks ", delta / tWeek);
			
	sprintf(ap->display, "%-8s %s %s %s  %5d  %s",
	ap->call, port_alias(ap->via), howlong, idle, ap->chat_port, ap->msg);
}

static void
add_process(int num, char *call, char *via, long ctime)
{
	struct active_users *ap = malloc_struct(active_users);
	int cnt = 0;

	if(ActiveUsers == NULL)
		ActiveUsers = ap;
	else {
		struct active_users *tap = ActiveUsers;
		cnt = 1;
		while(tap->next) {
			NEXT(tap);
			cnt++;
		}
		tap->next = ap;
	}

	ap->proc_num = num;
	strcpy(ap->via, via);
	ap->ctime = ctime;
	ap->idle = ctime+1;
	strcpy(ap->call, call);
	build_display(ap);

	if(!strcmp(via, "STATUS"))
		ap->window = NOWIN;
	else
#if 0
		if(!strcmp(via, "DAEMON")) {
			ap->window = DMNWIN;
			dmn_display();
		} else {
			ap->window = USRWIN;
			usr_display();
		}
#else
	ap->window = USRWIN;
	usr_add(ap, (ap->next)? ap->next->token:0);
#endif
}

static void
delete_process(int num)
{
	struct active_users *ap = ActiveUsers;

	if(ap->proc_num == num)
		ActiveUsers = ap->next;
	else {
		struct active_users *t;
		while(ap->next) {
			if(ap->next->proc_num == num) {
				t = ap;
				NEXT(ap);
				t->next = ap->next;
				break;
			}
			NEXT(ap);
		}

		if(ap->next == NULL)
			return;
	}

	switch(ap->window) {
#if 0
	case DMNWIN:
		dmn_delete(ap);
		break;
#endif
	case USRWIN:
		usr_delete(ap);
		break;
	}

	free(ap);
}

static void
update_process(int num, int port, long idle, int chat, char *s)
{
	struct active_users *ap = ActiveUsers;
	int cnt = 0;
	while(ap) {
		if(ap->proc_num == num)
			break;
		cnt++;
		NEXT(ap);
	}
	if(ap == NULL)
		return;

	if(port != ERROR)
		ap->chat_port = port;
	if(idle != ERROR)
		ap->idle = idle;
	if(chat != ERROR)
		ap->chat = chat;
	if(s) {
		strncpy(ap->msg, s, 200);
		ap->msg[200] = 0;
	}
	build_display(ap);

#if 0
	switch(ap->window) {
	case DMNWIN:
		dmn_display();
		break;
	case USRWIN:
		usr_display();
		break;
	}
#else
	usr_delete(ap);
	usr_add(ap, cnt);
#endif
}

static void
read_from_bbsd(caddr_t client_data, int *fd, XtInputId *id)
{
	char *s = bbsd_read();
	char *cmd;
	
	if(*s != '!')
		return;
	s+=2;

	cmd = get_string(&s);

	if(!strcmp(cmd, "LOGIN")) {
		char call[80];
		char via[80];
		int num;
		long ctime;
		
		num = get_number(&s);
		strcpy(call, get_string(&s));
		strcpy(via, get_string(&s));
		ctime = (long)get_number(&s);
		add_process(num, call, via, ctime);
		return;
	}

	if(!strcmp(cmd, "PING")) {
#if 0
		usr_display();
		dmn_display();
#endif
		return;
	}
		
	if(!strcmp(cmd, "LOGOUT")) {
		delete_process(get_number(&s));
		return;
	}

	if(!strcmp(cmd, "PORT")) {
		int num = get_number(&s);
		update_process(num, get_number(&s), ERROR, ERROR, NULL);
		return;
	}
	if(!strcmp(cmd, "MSG")) {
		int num = get_number(&s);
		update_process(num, ERROR, time(NULL), ERROR, s);
		return;
	}
	if(!strcmp(cmd, "CHAT")) {
		int num = get_number(&s);
		int port = get_number(&s);
		int chat = strcmp(get_string(&s), "ON")? FALSE:TRUE;
		update_process(num, port, ERROR, chat, NULL);
		return;
	}
#if 0
	if(!strcmp(cmd, "LOCK")) {
		lck_toggle(FALSE, get_string(&s), TRUE);
		return;
	}
	if(!strcmp(cmd, "UNLOCK")) {
		lck_toggle(FALSE, get_string(&s), FALSE);
		return;
	}
#endif
}

static void
init_status(void)
{
	struct text_line *t, *tl = NULL;
	bbsd_get_status(&tl);

	t = tl;
	while(t) {
		char *s = t->s;
		if(isdigit(*s)) {
			/* process info */
			int num, port, chat;
			long ctime, idle;
			char name[10];
			char via[10];

			num = get_number(&s);
			strcpy(name, get_string(&s));
			strcpy(via, get_string(&s));
			port = get_number(&s);
			ctime = (long)get_number(&s);
			idle = (long)get_number(&s);
			chat = strcmp(get_string(&s), "YES")? FALSE:TRUE;

			add_process(num, name, via, ctime);
			update_process(num, port, idle, chat, s);
		} else {
			/* lock info */
			char *via = get_string(&s);
			int lock = (*s == 'L') ? TRUE:FALSE;

#if 0
			lck_toggle(FALSE, via, lock);
#endif
		}
		NEXT(t);
	}
	textline_free(tl);
}

void
usage(char *pgm)
{
	printf("usage: %s [-h host] [-p port] [-w] [-d#]\n", pgm);
	printf("          -?\tdisplay help\n");
	printf("          -w\tdisplay required variables\n");
	printf("          -p bbsd_port_num [%d]\n", BBSD_PORT);
	printf("          -h bbsd_host [%s]\n", BBS_HOST);
	printf("          -d#\tdebugging options (hex)\n");
	printf("              100   leave daemon in foreground\n");
	printf("              200   ignore host\n");
	printf("              400   don't connect to other daemons\n");
	printf("\n");
}

main(int argc, char *argv[]) 
{
	Widget parent, panel;
	Widget button[BUTTONCNT];
	int i, bbsd_fd;
	static Position x = 225;
	static Position y = 10;
	Arg args[10];
	Cardinal n = 0;
	int retry = 0;

	parse_options(argc, argv, ConfigList,
		"BBSTOOL - OpenLook GUI interface to bbs and daemons");

	if(VERBOSE) {
		printf("bbsd_open(%s, %s, bbstool, %s)\n",
			Bbs_Host, Bbsd_Port, "STATUS");
		fflush(stdout);
	}

	while(bbsd_open(Bbs_Host, Bbsd_Port, "bbstool", "STATUS") == ERROR) {
		if(!(retry++ % 6))
			printf("Cannot connect to bbsd..... \n");
		sleep(5);
	}

	if(VERBOSE) {
		printf("Connected...\n");
		fflush(stdout);
	}

	if(!(dbug_level & dbgFOREGROUND))
		daemon();

	bbsd_fd = bbsd_socket();

	parent = XtInitialize(argv[0], "BBS", NULL, 0, &argc, argv);
	XtSetArg(args[n], XtNx, x); n++;
	XtSetArg(args[n], XtNy, y); n++;
	XtSetValues(parent, args, n);

	panel = XtVaCreateManagedWidget("panel",
		controlAreaWidgetClass, parent,
		XtNlayoutType,	OL_FIXEDCOLS,
		XtNmeasure,		1,
		NULL);

	for(i=0; i<BUTTONCNT; i++)
		button[i] = XtVaCreateManagedWidget(button_txt[i],
			checkBoxWidgetClass, panel,
			XtNposition,	OL_RIGHT,
			NULL);

	XtAddCallback(button[0], XtNselect, usr_callback, NULL);
	XtAddCallback(button[0], XtNunselect, usr_callback, NULL);
#if 0
	XtAddCallback(button[1], XtNselect, msg_callback, NULL);
	XtAddCallback(button[1], XtNunselect, msg_callback, NULL);
	XtAddCallback(button[2], XtNselect, dmn_callback, NULL);
	XtAddCallback(button[2], XtNunselect, dmn_callback, NULL);
	XtAddCallback(button[3], XtNselect, lck_callback, NULL);
	XtAddCallback(button[3], XtNunselect, lck_callback, NULL);
#endif
	XtAddCallback(button[4], XtNselect, quit_callback, NULL);

	usr_create_window();
#if 0
	msg_create_window();
	lck_create_window();
	dmn_create_window();
	init_status();
	bbsd_notify_on();

	XtAddInput(bbsd_fd, XtInputReadMask, read_from_bbsd, NULL);
#endif

	{
		long t0 = time(NULL);
		add_process(10, "N6ZFJ", "CONSOLE",  t0-120);
		add_process(11, "N0ARY", "CONSOLE",  t0-94);
		add_process(12, "N6UNE", "CONSOLE",  t0-15);
		update_process(11, 1234, t0-5, 0, "LIST LAST 1");
	}

	XtRealizeWidget(parent);
	XtMainLoop();
}

