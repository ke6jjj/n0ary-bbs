#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>
#ifndef	SABER
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

void
	get_socket(char *str, int cnt),
	putchar_socket(char *str);

void
#ifndef SABER
	print_socket(char *fmt, ...);
#else
	print_socket();
#endif

static void
	log_user(char *str);

#define DEFINITION		/* define config structures in this module */

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "tokens.h"
#include "user.h"
#include "message.h"
#include "help.h"
#include "callbk.h"
#include "function.h"
#include "history.h"
#include "vars.h"
#include "filesys.h"
#include "wp.h"
#include "remote.h"
#include "bbscommon.h"
#include "parse.h"
#include "cmd_pend.h"
#include "file.h"

static void
	init_program_vars(void);

static int
	/* change this to FALSE to see if the handshake is necessary.
	 * when set to TRUE the bbs will send a disconnect to tncd
	 * then pause waiting for tncd to kill me. With it set to 
	 * FALSE we simply close and exit.
	 */
	wait_to_die = FALSE,
	display_motd(int force);

static int socket_tnc_safe_raw_write(const char *buf);

short bbscallsum;

extern int
	LastMsgListedCdate;

FILE
	*logfile = NULL;

char
	*Via,
	prompt_string[4096];

time_t
	inactivity_timer = 0,
	time_now = 0;

int 
	batch_mode = FALSE,
	Program = ERROR,
	bbsd_sock = ERROR,
	sock = ERROR;

static int
	monitor_connected = FALSE,
	monitor_port = 0,
	monitor_sock = ERROR,
	monitor_fd = ERROR,
	escape_tnc_commands = FALSE;

#define	MONITOR_ON	"ON"
#define	MONITOR_OFF	"OFF"

int
monitor_setup()
{
	if((monitor_sock = socket_listen(NULL, &monitor_port)) == ERROR)
		monitor_port = ERROR; 
	return monitor_port;
}

void
monitor_disconnect()
{
	if(monitor_connected == FALSE)
		return;

	PRINTF("+-------------------------------------------------------------+\n");
	PRINTF("| The connection has been terminated, you are back to the bbs |\n");
	PRINTF("+-------------------------------------------------------------+\n");
	monitor_connected = FALSE;
}

void
monitor_connect()
{
	if(monitor_connected == TRUE)
		return;

	monitor_connected = TRUE;
	PRINTF("+-------------------------------------------------------+\n");
	PRINTF("| You are connected keyboard to keyboard with the sysop |\n");
	PRINTF("+-------------------------------------------------------+\n");
	bbsd_chat_cancel();
}

int
monitor_service()
{
	char buf[1024];

	if(socket_read_line(monitor_fd, buf, 1024, 2) == ERROR) {
		monitor_disconnect();
		return ERROR;
	}

	if(monitor_connected) {
		if(!strcmp(buf, MONITOR_OFF))
			monitor_disconnect();
		else
			PRINTF("%s\n", buf);
	} else
		if(!strcmp(buf, MONITOR_ON))
			monitor_connect();

	return OK;
}
	
static void
usage(char *pgm)
{
	printf("Usage:\n");
	printf("\t%s -h host -p port -t0 -c# -v via -s# -e -w [call]\n", pgm);
	printf("\t\t-h\tspecify new bbsd host\n");
	printf("\t\t-p\tspecify new bbsd port\n");
	printf("\t\t-d\tdon't daemonize, only used for testing\n");
	printf("\n");
	printf("\t\t-t\tprogram mode\n");
	printf("\t\t\t\t1 = bbs (default)\n");
	printf("\t\t\t\t2 = import\n");
	printf("\t\t\t\t3 = callbk server\n");
	printf("\t\t\t\t4 = wp server\n");
	printf("\t\t\t\t5 = voice remote\n");
	printf("\t\t\t\t6 = forwarding\n");
	printf("\n");
	printf("\t\t-t\tdebugging\n");
	printf("\t\t\t\t0x001 = tokens\n");
	printf("\t\t\t\t0x002 = translation\n");
	printf("\t\t\t\t0x004 = forwarding\n");
	printf("\t\t\t\t0x008 = user\n");
	printf("\t\t\t\t0x010 = message list\n");
	printf("\t\t\t\t0x800 = new (test)\n");
	printf("\n");
	printf("\t\t-c\tcommand level\n");
	printf("\t\t\t\t0 = all commands (default)\n");
	printf("\t\t\t\t1 = batch mode, no interactive commands\n");
	printf("\n");
	printf("\t\t-v\tconnect via (TNC0, TNC1, CONSOLE, PHONE, etc)\n");
	printf("\t\t-s\tsocket number (default to stdin/stdout)\n");
	printf("\t\t-U\timmediately enter SYSOP mode\n");
	printf("\t\t-u\tstart out in non-SYSOP mode (default)\n");
	printf("\t\t-e\tProtect against TNC escape sequences (~X commands)\n");
	printf("\t\t-w\tShow configuration (can be used multiple times)\n");
	printf("\n");
}

static void
read_options(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	int show_config = 0;
	int socket_number = 0;
	char *p, *config_fn, *to_whom, buf[1024];
	struct PortDefinition *pd = NULL;
	int c;

	if(argc == 1) {
		usage(argv[0]);
		exit(1);
	}

	Program = Prog_BBS;

	while((c = getopt(argc, argv, "d:eh:p:t:c:v:S:s:UuwWf:?")) != -1) {

		switch(c) {
		case 'e':
			escape_tnc_commands = TRUE;
			break;
		case 'h':
			Bbs_Host = optarg;
			break;
		case 'p':
			Bbsd_Port = atoi(optarg);
			break;

		case 'W':
			show_config++;
		case 'w':
			show_config++;
			break;
		case 'f':
			config_fn = optarg;
			break;

		case 'd':
			p = optarg;
			debug_level = get_hexnum(&p);
			break;

		case 't':
			Program = atoi(optarg);
			if(Program > Prog_FWD) {
				printf("-t invalid program request\n");
				exit(1);
			}
			break;

		case 'c':
			batch_mode = atoi(optarg);
			if(batch_mode != 0 && batch_mode != 1) {
				printf("-c should either be 0 or 1\n");
				batch_mode = 0;
			}	
			break;

		case 'v':
			uppercase(optarg);
			Via = copy_string(optarg);
			break;

		case 'U':
			ImSysop = ERROR;
			break;

		case 'u':
			ImSysop = TRUE;
			break;
		case 's':
			socket_number = atoi(optarg);
			break;

		case '?':
			usage(argv[0]);
			exit(0);
		}
	}
	if(show_config) {
		if(show_config > 1)
			show_configuration_rules(config_fn);
		show_reqd_configuration(ConfigList, "BBS program", config_fn);
		exit(0);
	}

		/* if we are talking via a socket, open it now */

	if(socket_number) {
		sock = socket_open(Bbs_Host, socket_number);
		if(sock < 0) {
			error_report(PRINT, TRUE);
			PRINT("Please report this to the sysop .... N0ARY/BBS\n");
			exit(1);
		}
	}

	switch(Program) {
	case Prog_IMPORT:
		if((argc - optind) != 1) {
			PRINT("Expected a callsign to be supplied\n");
			exit(1);
		}
		strcpy(usercall, "IMPORT");
		sprintf(buf, "IMPORT from %s", argv[optind]);
		if(Via == NULL)
			Via = "CONSOLE";
		break;

	case Prog_CALLBK:
		if((argc - optind) != 2) {
			PRINT("Expected a message number and the operation\n");
			exit(1);
		}
		strcpy(usercall, "CALLBK");
		if(Via == NULL)
			Via = "SERVER";
		break;

	case Prog_WP:
		if((argc - optind) != 2) {
			PRINT("Expected a message number and the operation\n");
			exit(1);
		}
		strcpy(usercall, "WP");
		if(Via == NULL)
			Via = "SERVER";
		break;

	case Prog_REMOTE:
		if((argc - optind) != 1) {
			PRINT("Expected a string to execute\n");
			exit(1);
		}
		strcpy(usercall, "REMOTE");
		if(Via == NULL)
			Via = "SERVER";
		break;

	case Prog_FWD:
			/* there are three options to forwarding.
			 *   1) bbs -t6            we must spawn the fwd processes
			 *   2) bbs -t6 -vTNC1     forward to all on this port
			 *   3) bbs -t6 call       forward to this bbs
			 */
		strcpy(usercall, "FWD");
		if(Via == NULL)
			Via = "SERVER";
		break;

	case Prog_BBS:
		if((argc - optind) != 1) {
			PRINT("Expected a callsign\n");
			exit(1);
		}
		strcpy(usercall, argv[optind]);
		uppercase(usercall);
		if(Via == NULL)
			Via = "CONSOLE";
		break;
	}

	if(bbsd_open(Bbs_Host, Bbsd_Port, usercall, Via) != OK) {
		error_report(PRINT, TRUE);
		PRINTF("%s@%s, Please try again later\n", usercall, Via);
		exit(1);
	}
	bbsd_pid();

	if(bbsd_get_configuration(ConfigList)) {
		error_report(PRINT, TRUE);
		PRINT("Please report this to the sysop and try again later\n");
		exit(1);
	}
#if 0
	smtp_set_port(Smtp_Port);
#endif

	time_now = bbsd_get_time();
	bbsd_sock = bbsd_socket();

	initialize_help_message();
	history_init(40);
	file_init();

	switch(Program) {
	case Prog_CALLBK:
#if 0
		daemon(1, 1);
#endif
		callbk_server(atoi(argv[optind]), argv[optind+1]);
		exit(0);

	case Prog_WP:
		daemon(1, 1);
		wp_server(atoi(argv[optind]), argv[optind+1]);
		exit(0);

	case Prog_REMOTE:
		remote_access(argv[optind]);
		exit(0);

	case Prog_FWD:
			/*   1) bbs -t6                 we must spawn the fwd processes
			 *   2) bbs -t6 -vTNC1 [call]   forward to all on this port
			 */
		pd = port_table();

		if(!strcmp(Via, "SERVER")) {
			while(pd) {
				switch(pd->type) {
				case tTNC:
				case tPHONE:
				case tSMTP:
					if(fork() == 0) {
						setsid();
						sprintf(buf,"%s/bin/b_bbs", Bbs_Dir);
						execl(buf, "b_bbs", "-t6", "-v", pd->name, 0);
						exit(1);
					}
				}
				NEXT(pd);
			}
			exit(0);
		}

		to_whom = NULL;
		if(argc != optind) {		
			to_whom = argv[optind];
			uppercase(to_whom);
		}

		while(msgd_open()) {
			PRINTF("timeout waiting for connection to msgd.\n");
			PRINTF("please try again later\n");
			exit_bbs();
		}

		msg_LoginUser(usercall);
		msg_BbsMode();
		build_full_message_list();

		while(pd) {
			if(!strcmp(pd->name, Via))
				if(debug_level & DBG_MSGFWD) {
					init_forwarding(Via, to_whom);
					exit(0);
				}
#if 0
				if(bbsd_check("FWD", Via) == 1) {
					init_forwarding(Via, to_whom);
					exit(0);
				}
#else
				init_forwarding(Via, to_whom);
				exit(0);
#endif
			NEXT(pd);
		}
		exit(0);
	}
}

/*ARGSUSED*/
int
main(int argc, char **argv)
{
	int retry = 0;
	char buf[4096];

		/* ease up on the permissions */

#ifndef SABER
	umask(0);
#endif
	prompt_string[0] = 0;
	Group[0] = 0;
	escape_tnc_commands = FALSE;

	read_options(argc, argv);

	bbscallsum = sum_string(Bbs_Call);

	switch(port_type(Via)) {
	case tPHONE:
		inactivity_timer = Bbs_Timer_Phone;
		break;
	case tTNC:
		inactivity_timer = Bbs_Timer_Tnc;
		break;
	case tCONSOLE:
		inactivity_timer = Bbs_Timer_Console;
		break;
	default:
		inactivity_timer = 10 * tMin;
	}

	if(login_user())
		exit(0);
	init_more();

		/* special case for importing message files */

	if(Program == Prog_IMPORT)
		ImBBS = TRUE;

	init_program_vars();

	if(monitor_setup() != ERROR)
		bbsd_port(monitor_port);

	while(msgd_open()) {
		if(!(retry % 3))
			PRINTF("waiting to connect to message process [msgd]\n");
		if(++retry > 24) {
			PRINTF("timeout waiting for connection to msgd.\n");
			PRINTF("please try again later\n");
			exit_bbs();
		}
		sleep(10);
	}

	msg_LoginUser(usercall);

				/* this is a special to handle elevating to sysop from
				 * the command line.  */
	if(ImSysop) {
		msg_SysopMode();
		if(ImSysop == ERROR) {
			msg_CatchUp();
			ImSysop = TRUE;
		}
	} else
		msg_NormalMode();

	if(!ImPotentialBBS) {
		display_motd(FALSE);
		build_full_message_list();
		if(batch_mode == FALSE)
			run_macro_zero();
	}

	while(TRUE) {
		error_report(PRINT, TRUE);

		if(prompt_string[0])
			PRINTF("**\n** %s\n**", prompt_string);

		if(batch_mode)
			PRINTF("%s>\n", usercall);
		else {
			if(ImBBS)
				PRINT(">\n");
			else {
				if(ImSysop)
					system_msg(61);
				else
					system_msg(60);
			}
		}

				/* do overhead here, we have displayed the prompt but it
				 * will take a while to reach the user and for him to 
				 * issue his next command, dead time.
				 */

		user_refresh();

		do {
			GETS(buf, 4095);
		} while(monitor_connected);

		if(buf[0] == '\n' || buf[0] == 0)
			continue;

		disable_help = FALSE;

		parse_command_line(buf);	

		run_pending_operations();
#ifdef THRASH_MSGD
		build_full_message_list();
#endif
	}

	return 0;
}

int reverse_fwd_mode = FALSE;

void
run_reverse_fwd(int fd)
{
	int oldsock = sock;
	char buf[4096];
	sock = fd;
	reverse_fwd_mode = TRUE;

	while(TRUE) {
		PRINTF("F>\n");
		GETS(buf, 4095);

		if(reverse_fwd_mode == ERROR)
			break;

		if(buf[0] == '\n' || buf[0] == 0)
			continue;

#if 0
		logd(buf);
#endif

		if(parse_command_line(buf) == ERROR)
			break;	

		if(reverse_fwd_mode == ERROR)
			break;

		run_pending_operations();
	}
	sock = oldsock;
	reverse_fwd_mode = FALSE;
}

void
exit_bbs(void)
{
	if(reverse_fwd_mode) {
		reverse_fwd_mode = ERROR;
		return;
	}

	if(ImBBS)
		logd_close();

	close_down_help_messages();
	if(LastMsgListedCdate)
		user_set_value(uMESSAGE, LastMsgListedCdate);

	logout_user();
	msgd_close();

	error_print();

	if(wait_to_die && sock != ERROR) {
		bbsd_msg("~B sent to tncd");
		socket_write(sock, "~B");
		pause();
	}

	if(sock != ERROR)
		socket_close(sock);
	exit(0);
}

static void
init_program_vars(void)
{
	last_message_listed = user_get_value(uMESSAGE);
}

/*ARGSUSED*/
int
motd(struct TOKEN *t)
{
	display_motd(TRUE);
	return OK;
}

static int
display_motd(int force)
{
	FILE *fp;
	char buf[80];

	if(!force) {
		if(last_login_time > file_last_altered(Bbs_Motd_File)) 
			return OK;
	}

	if((fp = fopen(Bbs_Motd_File, "r")) == NULL)
		return ERROR;

	while(fgets(buf, 80, fp))
		PRINT(buf);
	PRINT("\n");

	fclose(fp);

	return OK;
}


void
timeout_waiting_for_input()
{
	signal(SIGALRM, SIG_IGN);
	PRINTF("Timeout occured, link was idle for %d minutes.\n",
		inactivity_timer/60);
	exit_bbs();
}

void
get_socket(char *p, int cnt)
{
	char *str = p;
	fd_set ready;
	int fd = (sock == ERROR) ? 0:sock;
	int done = FALSE;
	int idletime = 0;


	if(socket_read_pending(fd) == TRUE) {
		if(socket_read_line(fd, str, cnt, 30) == sockERROR) {
			error_log("get_socket.read(): %s", sys_errlist[errno]);
			exit_bbs();
		}

		if(monitor_fd != ERROR)
			socket_raw_write(monitor_fd, p);

		if(ImLogging)
			log_user(p);
		return;
	}

		/* first check for input off of the bbsd port */

	while(!done) {
		struct timeval t;
		int result;
		int fdlimit = fd;

		FD_ZERO(&ready);
		FD_SET(fd, &ready);
		if(monitor_fd != ERROR) {
			FD_SET(monitor_fd, &ready);
			if(monitor_fd > fdlimit)
				fdlimit = monitor_fd;
		} else
			if(monitor_sock != ERROR) {
				FD_SET(monitor_sock, &ready);
				if(monitor_fd > fdlimit)
					fdlimit = monitor_fd;
			}
		FD_SET(bbsd_sock, &ready);
		if(bbsd_sock > fdlimit)
			fdlimit = bbsd_sock;

		fdlimit++;

		if(inactivity_timer) {
			t.tv_sec = PING_INTERVAL;
			t.tv_usec = 0;
			result = select(fdlimit, (void*)&ready, NULL, NULL, &t);
		} else
			result = select(fdlimit, (void*)&ready, NULL, NULL, NULL);

		switch(result) {
		case 0:
			idletime += PING_INTERVAL;
			if(idletime > inactivity_timer) {
				PRINTF("Timeout occured, link was idle for %d minutes.\n",
					inactivity_timer / 60);
				exit_bbs();
			}
#if 0
			bbsd_ping();
#endif
			continue;

		case ERROR:
			PRINTF("select: %s\n", sys_errlist[errno]);
			continue;
		}

		if(monitor_sock != ERROR)
			if(FD_ISSET(monitor_sock, &ready)) {
				char buf[80];
				if((monitor_fd = socket_accept(monitor_sock)) < 0)
					monitor_fd = ERROR;
				sprintf(buf, "Connected to %s\n", usercall);
				socket_raw_write(monitor_fd, buf);
				continue;
			}

		if(monitor_fd != ERROR)
			if(FD_ISSET(monitor_fd, &ready)) {
				if(monitor_service() == ERROR) {
					socket_close(monitor_fd);
					monitor_fd = ERROR;
				}
				continue;
			}

		if(FD_ISSET(bbsd_sock, &ready)) {
			char *c = bbsd_read();
			if(c == NULL) {
	PRINT("Lost connection with bbsd, please reconnect in a few minutes.\n");
				exit_bbs();
			}
			switch(*c++) {
			case 'P':
				strcpy(prompt_string, c);
				break;
			case 'I':
				PRINTF("%s\n", c);
				break;
			}
		}

		if(FD_ISSET(fd, &ready)) {
			if(socket_read_line(fd, str, cnt, 30) == sockERROR) {
				error_log("get_socket.read(): %s", sys_errlist[errno]);
				exit_bbs();
			}
			done = TRUE;
		}
	}

	if(monitor_fd != ERROR)
		socket_raw_write(monitor_fd, p);

	if(ImLogging)
		log_user(p);
}

void
putchar_socket(char *str)
{
	FILE *fp = stdout;

	if(Cnvrt2Uppercase)
		uppercase(str);

	if(ImLogging && ImBBS)
		log_user(str);

	if(monitor_fd != ERROR && monitor_connected == FALSE)
		socket_raw_write(monitor_fd, str);

	if(sock != ERROR) {
		if(socket_raw_write(sock, str) == ERROR)
			error_log("putchar_socket.write(): %s", sys_errlist[errno]);
	} else {
		while(*str)
			fputc(*str++, fp);
		fflush(fp);
	}
}

void
#ifndef SABER
print_socket(char *fmt, ...)
#else
print_socket(va_alist) va_dcl
#endif
{
	va_list ap;
	char buf[4096];
	int err;

#ifndef SABER
	va_start(ap, fmt);
#else
	char *fmt;
	va_start(ap);
	fmt = va_arg(ap, char*);
#endif
	vsnprintf(buf, sizeof(buf)-1, fmt, ap);
	va_end(ap);

	buf[sizeof(buf)-1] = '\0';

	if(Cnvrt2Uppercase)
		uppercase(buf);

	if(ImLogging)
		log_user(buf);

	if(monitor_fd != ERROR && monitor_connected == FALSE)
		socket_raw_write(monitor_fd, buf);

	if(sock != ERROR) {
		if (escape_tnc_commands)
			err = socket_tnc_safe_raw_write(buf);
		else
			err = socket_raw_write(sock, buf);

		if (err == ERROR)
			error_log("print_socket.write(): %s", sys_errlist[errno]);
	} else {
		printf("%s", buf);
		fflush(stdout);
	}
}

static void
log_user(char *str)
{
	if(logfile == NULL) {
		char fn[80];
		time_t t = Time(NULL);
		struct tm *dt = localtime(&t);

		sprintf(fn, "%s/%s", Bbs_Log_Path, usercall);
		if((logfile = fopen(fn, "a+")) == NULL)
			return;
		
		fprintf(logfile, "\n===========================\n");
		fprintf(logfile, "======[%02d/%02d @ %02d:%02d]======\n",
			dt->tm_mon+1, dt->tm_mday, dt->tm_hour, dt->tm_min);
		fprintf(logfile, "===========================\n");
	}

	if(index(str, '\n'))
		fprintf(logfile, "%s", str);
	else
		fprintf(logfile, "%s\n", str);
	fflush(logfile);
}

int
ports(void)
{
	time_t t1 = time(NULL);
	struct text_line *tl, *TL = NULL;
	bbsd_get_status(&TL);


	tl = TL;
	PRINT("                   ------ Time -----\n");
	PRINT(" Call     Via      Connect    Idle\n");
	while(tl) {
		char call[20], via[20];
		int hours, minutes, seconds, delta;
		int ihours, iminutes, iseconds, idle;
		int pid;
		char *s = tl->s;

		if(!isdigit(*s))
			break;
		
		get_number(&s);	/* proc number, skip over */
		strcpy(call, get_string(&s));
		strcpy(via, get_string(&s));
		get_number(&s); /* chat port, skip over */
		pid = get_number(&s);
		delta = t1 - get_number(&s);
		idle = t1 - get_number(&s);

		hours = delta/3600;
		delta %= 3600;
		minutes = delta/60;
		seconds = delta%60;

		ihours = idle/3600;
		idle %= 3600;
		iminutes = idle/60;
		iseconds = idle%60;

		if(port_type(via) || ImSysop)
			PRINTF("%-8s %-8s %2d:%02d:%02d  %2d:%02d:%02d\n",
				call, port_alias(via), hours, minutes, seconds,
					ihours, iminutes, iseconds);
		NEXT(tl);
	}
	textline_free(TL);
	return OK;
}

int
chat(void)
{
	PRINT("The sysop has been paged. The request will be displayed on the\n");
	PRINT("console for the duration of your session. If the sysop is\n");
	PRINT("available you will drop into keyboard to keyboard mode.\n");
	PRINT("For now just continue with your bbs activity.\n\n");

	bbsd_chat_request();
	return OK;
}

time_t
Time(time_t *t)
{
	time_t now = time_now;

	if(now == 0)
		now = time(NULL);

	if(t != NULL)
		*t = now;
	return now;
}

/*
 * Write a string to the user output socket in such a way that no in-band
 * TNC commands will appear in the stream (this is a requirement when talking
 * to an AX.25 connection that is mediated by tncd).
 */
static int
socket_tnc_safe_raw_write(const char *buf)
{
	static int tnc_command_state = 0;
	const char *unsafe;
	ssize_t err;

	if (tnc_command_state == 1 && buf[0] == '~') {
		err = socket_raw_write_n(sock, "~", 1);
		if (err == ERROR)
			return ERROR;
		tnc_command_state = 0;
	}

	while ((unsafe = strstr(buf, "\n~")) != NULL) {
		err = socket_raw_write_n(sock, buf, unsafe-buf+2);
		if (err == ERROR)
			return ERROR;
		err = socket_raw_write_n(sock, "~", 1);
		if (err == ERROR)
			return ERROR;
		buf = unsafe+2;
	}

	if (buf[0] != '\0') {
		err = socket_raw_write(sock, buf);
		if (err == ERROR)
			return ERROR;
		size_t len = strlen(buf);
		tnc_command_state = buf[len-1] == '\n';
	}

	return 0;
}
