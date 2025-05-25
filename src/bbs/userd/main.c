#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "alib.h"
#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "user.h"

struct UserDirectory *UsrDir = NULL;

time_t
	time_now = 0,
	Userd_Age_Suspect,
	Userd_Age_Home,
	Userd_Age_NotHome,
	Userd_Age_NonHam,
	Userd_Age_Interval = (1 * tHour);

int
	Userd_Port;

char
	*Bbs_Call,
	*Userd_Acc_Path,
	*Userd_Bind_Addr = NULL;

struct active_processes *procs = NULL;
int shutdown_daemon = FALSE;
char output[4096];

struct ConfigurationList ConfigList[] = {
	{ "",					tCOMMENT,	NULL },
	{ "BBS_CALL",			tSTRING,	(int*)&Bbs_Call },
	{ "BBS_HOST",			tSTRING,	(int*)&Bbs_Host },
	{ "BBSD_PORT",			tINT,		(int*)&Bbsd_Port },
	{ "",					tCOMMENT,	NULL },
	{ "USERD_PORT",			tINT,		(int*)&Userd_Port },
	{ "USERD_BIND_ADDR",		tSTRING,	(int*)&Userd_Bind_Addr },
	{ "USERD_ACC_PATH",		tDIRECTORY,	(int*)&Userd_Acc_Path },
	{ "USERD_AGE_INTERVAL",	tTIME,		(int*)&Userd_Age_Interval },
	{ "USERD_AGE_SUSPECT",	tTIME,		(int*)&Userd_Age_Suspect },
	{ "USERD_AGE_HOME",		tTIME,		(int*)&Userd_Age_Home },
	{ "USERD_AGE_NONHAM",	tTIME,		(int*)&Userd_Age_NonHam },
	{ "USERD_AGE_NOTHOME",	tTIME,		(int*)&Userd_Age_NotHome },
	{ NULL, 0, NULL}};

static void accept_callback(void *ctx, void *arg0, int arg1);
static void service_callback(void *ctx, void *arg0, int arg1);
static void msg_age_callback(void *ctx, void *arg0, int arg1);
static void bbsd_callback(void *ctx, void *arg0, int arg1);

static int
service_port(struct active_processes *ap)
{
	char *c, *s, *buf;
	int have_line;

	for (have_line = 0; ap->sz < sizeof(ap->buf);) {
		c = &ap->buf[ap->sz];
		if (read(ap->fd, c, 1) <= 0) {
			if (errno == EAGAIN) {
				/* No more data available right now */
				return OK;
			}
			return ERROR;
		}
		if(*c == '\r')
			continue;

		if(*c == '\n') {
			*c = 0;
			have_line = 1;
			break;
		}

		ap->sz++;
	}

	if (have_line != 1) {
		/* peer has spewed too much garbage */
		return ERROR;
	}

	ap->sz = 0;
	buf = ap->buf;

	log_f("userd", "R:", buf);

	s = buf;
	NextChar(s);
	if(*s) {
		if((c = parse(ap, s)) == NULL) {
			return ERROR;
		}
	} else
		c = "OK\n";

	if(socket_raw_write(ap->fd, c) == ERROR)
		return ERROR;

	log_f("userd", "S:", c);
	return OK;
}

static struct active_processes *
add_proc()
{
    struct active_processes
        *tap = procs,
        *ap = malloc_struct(active_processes);

    if(tap == NULL)
        procs = ap;
    else {
        while(tap->next)
            NEXT(tap);
        tap->next = ap;
    }

    ap->fd = ERROR;
    ap->ev = NULL;
    ap->sz = 0;
    return ap;
}

static struct active_processes *
remove_proc(struct active_processes *ap)
{
    struct active_processes *tap = procs;

	usrfile_close_all(ap);
    if(procs == ap) {
        NEXT(procs);
		tap = procs;
    } else {
        while(tap) {
            if(tap->next == ap) {
                tap->next = tap->next->next;
				NEXT(tap);
                break;
            }
            NEXT(tap);
        }
    }    

    if (ap->ev != NULL) {
        alEvent_deregister(ap->ev);
    }
    socket_close(ap->fd);
    free(ap);
    return tap;
}

int
main(int argc, char *argv[])
{
	int listen_port;
	int listen_sock;
	int bbsd_sock;
	int res;
	char *bind_addr;
	alCallback cb;
	alEventHandle accept_ev, bbsd_ev;

	parse_options(argc, argv, ConfigList, "USERD - User Daemon");

	if(dbug_level & dbgTESTHOST)
		test_host(Bbs_Host);
	if(!(dbug_level & dbgFOREGROUND))
		daemon(1, 1);

    if(bbsd_open(Bbs_Host, Bbsd_Port, "userd", "DAEMON") == ERROR)
		error_print_exit(0);
	error_clear();

	bbsd_get_configuration(ConfigList);
    bbsd_msg("Startup");
	bbsd_sock = bbsd_socket();
	time_now = bbsd_get_time();

	usrdir_build();

	if(dbug_level & dbgVERBOSE)
		printf("UP\n");

	listen_port = Userd_Port;
	bind_addr = Userd_Bind_Addr;
	if((listen_sock = socket_listen(bind_addr, &listen_port)) == ERROR)
		return 1;

    bbsd_port(Userd_Port);
	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("");

	signal(SIGPIPE, SIG_IGN);

	if (alEvent_init() != 0) {
		fprintf(stderr, "Unable to initialize event system\n");
		return 1;
	}

	AL_CALLBACK(&cb, (void*)bbsd_sock, bbsd_callback);
	if (alEvent_registerFd(bbsd_sock, ALFD_READ, cb, &bbsd_ev) != 0) {
		fprintf(stderr, "Unable to register bbsd activity socket\n");
		return 1;
	}

	AL_CALLBACK(&cb, (void*)listen_sock, accept_callback);
	if (alEvent_registerFd(listen_sock, ALFD_READ, cb, &accept_ev) != 0) {
		fprintf(stderr, "Unable to register listen socket\n");
		return 1;
	}

	AL_CALLBACK(&cb, NULL, msg_age_callback);
	res = alEvent_addTimer(Userd_Age_Interval*1000, ALTIMER_REPEAT, cb);
	if (res != 0) {
		fprintf(stderr, "Unable to register aging timer\n");
		return 1;
	}

	while (!shutdown_daemon && alEvent_pending()) {
		alEvent_poll();
	}

	return 0;
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

static void
accept_callback(void *ctx, void *arg0, int arg1)
{
	struct active_processes *ap;
	int fd, res, listen_sock;
	alCallback cb;

	listen_sock = (int) ctx;
	fd = socket_accept(listen_sock);
	if (fd < 0) {
		fprintf(stderr, "accept failed on new connection.\n");
		return;
	}

	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
		fprintf(stderr, "unable to make new connection non-blocking\n");
		close(fd);
		return;
	}

	if (socket_write(fd, daemon_version("userd", Bbs_Call)) == ERROR) {
		fprintf(stderr, "couldn't write hello banner.\n");
		close(fd);
		return;
	}

	ap = add_proc();
	ap->fd = fd;

	AL_CALLBACK(&cb, ap, service_callback);
	res = alEvent_registerFd(fd, ALFD_READ, cb, &ap->ev);
	if (res != 0) {
		fprintf(stderr, "couldn't register new process\n");
		remove_proc(ap);
	}
}

static void
service_callback(void *ctx, void *arg0, int arg1)
{
	struct active_processes *ap = (struct active_processes *) ctx;

	if (service_port(ap) == ERROR) {
		remove_proc(ap);
	}
}

static void
msg_age_callback(void *ctx, void *arg0, int arg1)
{
	log_clear("userd");
	log_f("userd", "X:", "Start aging");
	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("Aging users");
	age_users(NULL);
	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("");
}

static void
bbsd_callback(void *ctx, void *arg0, int arg1)
{
	shutdown_daemon = TRUE;
}
