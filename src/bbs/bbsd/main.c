#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "alib.h"
#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bbsd.h"
#include "daemon.h"

long
	time_now = 0;

int
	StatusPort = ERROR,
	shutdown_daemon = FALSE,
	proc_num = 1;

char
	*Bbs_Call,
	*Bbsd_Bind_Addr = NULL,
	*dummy_str,
	output[4096];

struct text_line
	*Notify = NULL;

struct active_processes *procs = NULL;

struct daemon_check_context {
	int ping_interval;
	int starting_up;
};

struct ConfigurationList ConfigList[] = {
	{ "",					tCOMMENT,	NULL },
	{ "BBS_HOST",			tSTRING,	(int*)&Bbs_Host },
	{ "BBSD_PORT",			tINT,		(int*)&Bbsd_Port },
	{ "BBSD_BIND_ADDR",		tSTRING,	(int*)&Bbsd_Bind_Addr },
	{ "",					tCOMMENT,	NULL },
	{ " Used by libbbs",	tCOMMENT,	NULL },
	{ "",					tCOMMENT,	NULL },
	{ "BBS_CALL",			tSTRING,	(int*)&Bbs_Call },
	{ "BBS_DOMAIN",			tSTRING,	(int*)&dummy_str },
	{ NULL, 0, NULL}};

static void accept_callback(void *ctx, void *arg0, int arg1);
static void service_callback(void *ctx, void *arg0, int arg1);
static void daemon_check_callback(void *ctx, void *arg0, int arg1);
static int next_check_delay_s(int ping_interval, int starting_up);
static void notify(char *msg);
static void notify_callback(void *ctx, void *arg0, int arg1);

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
	if(VERBOSE)
		printf("R: %s\n", buf);

	log_f("bbsd", "R:", buf);
	s = buf;
	NextChar(s);
	if(*s) {
		if((c = parse(ap, s)) == NULL)
			return ERROR;
	} else
		c = Error("invalid command");

	log_f("bbsd", "S:", c);
	if(VERBOSE)
		printf("S: %s", c);

	if(*c != 0)
		if(write(ap->fd, c, strlen(c)) < 0)
			return ERROR;

	return OK;
}

static struct active_processes *
add_proc()
{
	alCallback cb;
	char buf[256];
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

	strcpy(ap->call, "WHO?");
	ap->via = NULL;
	ap->t0 = ap->idle = bbs_time(NULL);
	ap->fd = ERROR;
	ap->proc_num = proc_num++;
	ap->pid = ERROR;
	ap->verbose = FALSE;
	ap->sz = 0;
	ap->ev = NULL;

	snprintf(buf, sizeof(buf),
		"LOGIN %d UnKwn STATUS %ld", ap->proc_num, ap->t0);
	notify(buf);

	return ap;	
}

static struct active_processes *
remove_proc(struct active_processes *ap)
{
	struct active_processes *tap = procs;
	char buf[80];

	if((long)procs == (long)ap) {
		NEXT(procs);
		tap = procs;
	} else {
		while(tap) {
			if((long)tap->next == (long)ap) {
				tap->next = tap->next->next;
				NEXT(tap);
				break;
			}
			NEXT(tap);
		}
	}

	sprintf(buf, "LOGOUT %d", ap->proc_num);
	notify(buf);

	daemon_check_out(ap);

	if (ap->ev != NULL) {
		alEvent_deregister(ap->ev);
	}

	close(ap->fd);
	free(ap);
	return tap;
}

static void
ping_monitors(void)
{
	struct active_processes *ap = procs;
	char *ping = "! PING\n";

	while(ap) {
		if(ap->notify)
			if(write(ap->fd, ping, strlen(ping)) < 0) {
				ap = remove_proc(ap);
				continue;
			}
		NEXT(ap);
	}
}

static void
notify_monitors(void)
{
	struct text_line *ntl = Notify, *tl = Notify;
		/* start new list so processes that disappear during this
		 * routine will not be forgotten.
		 */
	Notify = NULL;

	while(tl) {
		struct active_processes *ap = procs;
		char buf[256];
		sprintf(buf, "! %s\n", tl->s);

		while(ap) {
			if(ap->notify)
				if(write(ap->fd, buf, strlen(buf)) < 0) {
					ap = remove_proc(ap);
					continue;
				}
			NEXT(ap);
		}
		NEXT(tl);
	}

	textline_free(ntl);
}

static int
check_for_bbsd(void)
{
	if(bbsd_open(Bbs_Host, Bbsd_Port, "BBSD", "STATUS") == ERROR) {
		error_clear();
		return OK;
	}

	return ERROR;
}

static int
fetch_configuration(void)
{
	char *s;
	int fail = 0;

	if((s = config_fetch("BBS_HOST")) != NULL)
		Bbs_Host = s;

	if((s = config_fetch("BBS_CALL")) != NULL)
		Bbs_Call = s;

	if((s = config_fetch("BBSD_PORT")) != NULL) {
		if(!isdigit(*s)) {
			printf("BBSD_PORT expected a number, got %s\n", s);
			fail++;
		} else
			Bbsd_Port = atoi(s);
	}
	if ((s = config_fetch("BBSD_BIND_ADDR")) != NULL)
		Bbsd_Bind_Addr = s;

	return fail;
}

int
main(int argc, char *argv[])
{
	int listen_port;
	char *listen_addr;
	int listen_sock, delay_s;
	alEventHandle listen_ev;
        alCallback cb;
	struct active_processes *ap;
	struct daemon_check_context check_ctx;

	extern char *config_fn;
	extern int optind;

	parse_options(argc, argv, ConfigList, "BBSD - BBS Super Daemon");

	if(read_config_file(argv[optind]) == ERROR) {
		printf("Couldn't read configuration file %s\n", argv[optind]);
		return 1;
	}
	if(fetch_configuration() != OK) {
		printf("Critical failures occured with configuration, aborting..\n");
		return 1;
	}

	if(check_for_bbsd()) {
		printf("Already a bbsd process running\n");
		return 0;
	}

	if(dbug_level & dbgTESTHOST)
		test_host(Bbs_Host);

	if(!(dbug_level & dbgFOREGROUND))
		daemon(1, 1);
	signal(SIGPIPE, SIG_IGN);

	lock_all("Bring up in progress");

	listen_port = Bbsd_Port;
	listen_addr = Bbsd_Bind_Addr;
	if((listen_sock = socket_listen(listen_addr, &listen_port)) == ERROR) {
		printf("Unabled to listen on bbsd port.\n");
		return 1;
	}

	build_daemon_list();

	if (alEvent_init() != 0) {
		fprintf(stderr, "Unable to initialize event system\n");
		return 1;
	}

	AL_CALLBACK(&cb, (void *)listen_sock, accept_callback);
	if (alEvent_registerFd(listen_sock, ALFD_READ, cb, &listen_ev) != 0) {
		fprintf(stderr, "Unable to register listen socket\n");
		return 1;
	}

	check_ctx.ping_interval = PING_INTERVAL;
	check_ctx.starting_up = TRUE;
	delay_s = next_check_delay_s(PING_INTERVAL, TRUE);
	AL_CALLBACK(&cb, &check_ctx, daemon_check_callback);
	if (alEvent_addTimer(delay_s * 1000, 0, cb) < 0) {
		fprintf(stderr, "Unable to set up daemon timer\n");
		return 1;
	}

	while (!shutdown_daemon && alEvent_pending()) {
		alEvent_poll();
	}

	return 0;
}

long
bbs_time(long *t)
{
	long now;

	if(time_now != 0) {
		char *result = config_fetch("TIME");
		if(result != NULL)
			time_now = atol(result);
		else
			time_now = 0;
	}

	now = time_now;

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
	char buf[80];
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

	if (socket_write(fd, daemon_version("bbsd", Bbs_Call)) != sockOK) {
		fprintf(stderr, "couldn't write hello banner.\n");
		close(fd);
		return;
	}

	ap = add_proc();
	ap->fd = fd;

	snprintf(buf, sizeof(buf), "# %d\n", ap->proc_num);
	if(write(fd, buf, strlen(buf)) < 0) {
		fprintf(stderr, "couldn't write second hello banner.\n");
		remove_proc(ap);
	}

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

	if(service_port(ap) == ERROR) {
		remove_proc(ap);
	}
}


static int
next_check_delay_s(int ping_interval, int starting_up)
{
	if(Notify || starting_up)
		return 2;
	else
		return ping_interval;
}

static void
daemon_check_callback(void *_ctx, void *arg0, int arg1)
{
	struct daemon_check_context *ctx;
	int delay_s, res;
	alCallback cb;

	ctx = (struct daemon_check_context *) _ctx;

	ping_monitors();

	ctx->starting_up = daemons_start();

	delay_s = next_check_delay_s(ctx->ping_interval, ctx->starting_up);
	
	AL_CALLBACK(&cb, ctx, daemon_check_callback);
	res = alEvent_addTimer(delay_s * 1000, 0, cb);
	if (res < 0) {
		fprintf(stderr, "can't register daemon check timer\n");
		exit(1);
	}
}

static void
notify(char *msg)
{
	alCallback cb;

	textline_append(&Notify, msg);
	AL_CALLBACK(&cb, NULL, notify_callback);
	if (alEvent_queueCallback(cb, ALCB_UNIQUE,  NULL, 0) != 0) {
		fprintf(stderr, "unable to queue notify callback\n");
		exit(1);
	}
}

static void
notify_callback(void *ctx, void *arg0, int arg1)
{
	if (Notify == NULL)
		return;

	notify_monitors();
}
