#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
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

struct proc_list procs = LIST_HEAD_INITIALIZER(procs);

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
static int handle_line(struct active_process *ap, char *buf);

static int
service_port(struct active_process *ap)
{
	char *buf;
	int res;

	for (;;) {
		res = async_line_get(ap->buf, ap->fd, &buf);
		if (res == ASYNC_MORE)
			return OK;

		if (res != ASYNC_OK)
			return ERROR;

		if (handle_line(ap, buf) != OK)
			return ERROR;
	}
}

static int
handle_line(struct active_process *ap, char *buf)
{
	char *c, *s;

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

static struct active_process *
add_proc()
{
	alCallback cb;
	char buf[256];
	struct active_process *ap = malloc_struct(active_process);

	if (ap == NULL)
		goto AllocFailed;

	ap->buf = async_line_new(1024, 1 /*Filter CR*/);
	if (ap->buf == NULL)
		goto AsyncAllocFailed;

	ap->via = NULL;
	ap->t0 = ap->idle = bbs_time(NULL);
	ap->fd = ERROR;
	ap->proc_num = proc_num++;
	ap->pid = ERROR;
	ap->verbose = FALSE;
	ap->ev = NULL;
	LIST_INSERT_HEAD(&procs, ap, entries);

	strcpy(ap->call, "WHO?");

	snprintf(buf, sizeof(buf),
		"LOGIN %d UnKwn STATUS %ld", ap->proc_num, ap->t0);
	notify(buf);

	return ap;

AsyncAllocFailed:
	free(ap);
AllocFailed:
	return NULL;
}

static void
remove_proc(struct active_process *ap)
{
	char buf[80];

	LIST_REMOVE(ap, entries);

	sprintf(buf, "LOGOUT %d", ap->proc_num);
	notify(buf);

	daemon_check_out(ap);

	if (ap->ev != NULL) {
		alEvent_deregister(ap->ev);
	}

	close(ap->fd);
	async_line_free(ap->buf);
	free(ap);
}

static void
ping_monitors(void)
{
	struct active_process *ap, *ap_temp;
	char *ping = "! PING\n";

	LIST_FOREACH_SAFE(ap, &procs, entries, ap_temp) {
		if(ap->notify)
			if(write(ap->fd, ping, strlen(ping)) < 0) {
				remove_proc(ap);
			}
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
		struct active_process *ap, *ap_temp;
		char buf[256];
		sprintf(buf, "! %s\n", tl->s);

		LIST_FOREACH_SAFE(ap, &procs, entries, ap_temp) {
			if(ap->notify)
				if(write(ap->fd, buf, strlen(buf)) < 0) {
					remove_proc(ap);
				}
		}
		NEXT(tl);
	}

	textline_free(ntl);
}

static int
check_for_bbsd(void)
{
	if(bbsd_open(Bbs_Host, Bbsd_Port, "BBSD", "STATUS") == ERROR) {
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
	intptr_t listen_sock;
	int delay_s;
	alEventHandle listen_ev;
        alCallback cb;
	struct daemon_check_context check_ctx;

	extern char *config_fn;
	extern int optind;

	bbs_log_init("b_bbsd", 1 /* Also log to stderr */);

	parse_options(argc, argv, ConfigList, "BBSD - BBS Super Daemon");

	if (dbug_level & dbgVERBOSE)
		bbs_log_level(BBS_LOG_DEBUG);

	if(read_config_file(argv[optind]) == ERROR) {
		log_error("Couldn't read configuration file %s", argv[optind]);
		return 1;
	}
	if(fetch_configuration() != OK) {
		log_error("Critical failures occured with configuration, aborting..");
		return 1;
	}

	if(check_for_bbsd()) {
		log_warning("Already a bbsd process running");
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
		log_error("Unable to listen on bbsd port.");
		return 1;
	}

	build_daemon_list();

	if (alEvent_init() != 0) {
		log_error("Unable to initialize event system");
		return 1;
	}

	AL_CALLBACK(&cb, (void *)listen_sock, accept_callback);
	if (alEvent_registerFd(listen_sock, ALFD_READ, cb, &listen_ev) != 0) {
		log_error("Unable to register listen socket");
		return 1;
	}

	check_ctx.ping_interval = PING_INTERVAL;
	check_ctx.starting_up = TRUE;
	delay_s = next_check_delay_s(PING_INTERVAL, TRUE);
	AL_CALLBACK(&cb, &check_ctx, daemon_check_callback);
	if (alEvent_addTimer(delay_s * 1000, 0, cb) < 0) {
		log_error("Unable to set up daemon timer");
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
	struct active_process *ap;
	int fd, res, flags;
	intptr_t listen_sock;
	char buf[80];
	alCallback cb;

	listen_sock = (intptr_t) ctx;
	fd = socket_accept_nonblock_unmanaged(listen_sock);
	if (fd < 0) {
		log_error("accept failed on new connection.");
		goto AcceptFailed;
	}

	if (socket_write(fd, daemon_version("bbsd", Bbs_Call)) != sockOK) {
		log_error("couldn't write hello banner.");
		goto BannerFailed;
	}

	ap = add_proc();
	ap->fd = fd;

	snprintf(buf, sizeof(buf), "# %d\n", ap->proc_num);
	if(write(fd, buf, strlen(buf)) < 0) {
		log_error("couldn't write second hello banner.");
		goto SecondBannerFailed;
	}

	AL_CALLBACK(&cb, ap, service_callback);
	res = alEvent_registerFd(fd, ALFD_READ, cb, &ap->ev);
	if (res != 0) {
		log_error("couldn't register new process");
		goto RegisterFailed;
	}

	return;

RegisterFailed:
SecondBannerFailed:
	remove_proc(ap);
	return;
BannerFailed:
	close(fd);
AcceptFailed:
	return;
}

static void
service_callback(void *ctx, void *arg0, int arg1)
{
	struct active_process *ap = (struct active_process *) ctx;

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
		log_error_exit(1, "can't register daemon check timer");
	}
}

static void
notify(char *msg)
{
	alCallback cb;

	textline_append(&Notify, msg);
	AL_CALLBACK(&cb, NULL, notify_callback);
	if (alEvent_queueCallback(cb, ALCB_UNIQUE,  NULL, 0) != 0) {
		log_error_exit(1, "unable to queue notify callback");
	}
}

static void
notify_callback(void *ctx, void *arg0, int arg1)
{
	if (Notify == NULL)
		return;

	notify_monitors();
}
