#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/ArrowBG.h>
#include <Xm/CascadeB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/MainW.h>
#include <Xm/PushB.h>
#include <Xm/Scale.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bbstool.h"

#define BUTTONCNT	5

char
	*Bbs_Call;

struct my_active_procs *ActiveProcs = NULL;

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

extern void msg_callback();

void
quit_callback(Widget widget, caddr_t client_data, caddr_t call_data)
{

	Display *display;
	display = XtDisplay(widget);
	XtCloseDisplay(display);

	while(ActiveProcs) {
		struct my_active_procs *ap = ActiveProcs;
		NEXT(ActiveProcs);
		free(ap);
	}

	exit(0);
}

build_display(struct my_active_procs *ap) 
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
	struct my_active_procs *ap = malloc_struct(my_active_procs);

	if(ActiveProcs == NULL)
		ActiveProcs = ap;
	else {
		struct my_active_procs *tap = ActiveProcs;
		while(tap->next)
			NEXT(tap);
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
		if(!strcmp(via, "DAEMON")) {
			ap->window = DMNWIN;
			dmn_display();
		} else {
			ap->window = USRWIN;
			usr_display();
		}
}

static void
delete_process(int num)
{
	struct my_active_procs *ap = ActiveProcs;

	if(ap->proc_num == num)
		ActiveProcs = ap->next;
	else {
		struct my_active_procs *t;
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
	case DMNWIN:
		dmn_display();
		break;
	case USRWIN:
		usr_display();
		break;
	}
	free(ap);
}

static void
update_process(int num, int port, int pid, long idle, int chat, char *s)
{
	struct my_active_procs *ap = ActiveProcs;
	while(ap) {
		if(ap->proc_num == num)
			break;
		NEXT(ap);
	}
	if(ap == NULL)
		return;

	if(port != ERROR)
		ap->chat_port = port;
	if(pid != ERROR)
		ap->pid = pid;
	if(idle != ERROR)
		ap->idle = idle;
	if(chat != ERROR)
		ap->chat = chat;
	if(s) {
		strncpy(ap->msg, s, 200);
		ap->msg[200] = 0;
	}
	build_display(ap);

	switch(ap->window) {
	case DMNWIN:
		dmn_display();
		break;
	case USRWIN:
		usr_display();
		break;
	}
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
		struct my_active_procs *ap = ActiveProcs;
		while(ap) {
			build_display(ap);
			NEXT(ap);
		}
		usr_display();
		dmn_display();
		return;
	}
		
	if(!strcmp(cmd, "LOGOUT")) {
		delete_process(get_number(&s));
		return;
	}

	if(!strcmp(cmd, "PORT")) {
		int num = get_number(&s);
		update_process(num, get_number(&s), ERROR, ERROR, ERROR, NULL);
		return;
	}
	if(!strcmp(cmd, "MSG")) {
		int num = get_number(&s);
		update_process(num, ERROR, ERROR, time(NULL), ERROR, s);
		return;
	}
	if(!strcmp(cmd, "CHAT")) {
		int num = get_number(&s);
		int port = get_number(&s);
		int chat = strcmp(get_string(&s), "ON")? FALSE:TRUE;
		update_process(num, port, ERROR, ERROR, chat, NULL);
		return;
	}
	if(!strcmp(cmd, "LOCK")) {
		lck_toggle(FALSE, get_string(&s), TRUE);
		return;
	}
	if(!strcmp(cmd, "UNLOCK")) {
		lck_toggle(FALSE, get_string(&s), FALSE);
		return;
	}
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
			long pid, ctime, idle;
			char name[10];
			char via[10];

			num = get_number(&s);
			strcpy(name, get_string(&s));
			strcpy(via, get_string(&s));
			port = get_number(&s);
			pid = get_number(&s);
			ctime = (long)get_number(&s);
			idle = (long)get_number(&s);
			chat = strcmp(get_string(&s), "YES")? FALSE:TRUE;

			add_process(num, name, via, ctime);
			update_process(num, port, pid, idle, chat, s);
		} else {
			/* lock info */
			char *via = get_string(&s);
			int lock = (*s == 'L') ? TRUE:FALSE;

			lck_toggle(FALSE, via, lock);
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
		"BBSTOOL - Motif GUI interface to bbs and daemons");

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
/*
	bbsd_pid();
*/

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
		xmRowColumnWidgetClass, parent,
		XmNwidth, 200,
		NULL);

	for(i=0; i<BUTTONCNT; i++)
		button[i] = XtVaCreateManagedWidget(button_txt[i],
			xmToggleButtonWidgetClass, panel, NULL);

	XtAddCallback(button[0], XmNvalueChangedCallback, usr_callback, NULL);
	XtAddCallback(button[1], XmNvalueChangedCallback, msg_callback, NULL);
	XtAddCallback(button[2], XmNvalueChangedCallback, dmn_callback, NULL);
	XtAddCallback(button[3], XmNvalueChangedCallback, lck_callback, NULL);
	XtAddCallback(button[4], XmNvalueChangedCallback, quit_callback, NULL);

	usr_create_window();
	msg_create_window();
	lck_create_window();
	dmn_create_window();

	init_status();
	bbsd_notify_on();

	XtAddInput(bbsd_fd, XtInputReadMask, read_from_bbsd, NULL);

	XtRealizeWidget(parent);
	XtMainLoop();
}

