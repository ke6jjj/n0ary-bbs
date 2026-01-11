#include <sys/queue.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
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
#include "bid.h"

time_t
	time_now = 0;

int
	Bidd_Age,
	Bidd_Flush;

int
	Bidd_Port;

static const int Log_Clear_Interval_s = 60;

char
    *Bbs_Call,
	*Bidd_Bind_Addr = NULL,
	*Bidd_File;

char output[4096];
int shutdown_daemon = FALSE;

struct active_process {
	LIST_ENTRY(active_process) entries;
	struct AsyncLineBuffer *buf;
	int fd;
	alEventHandle ev;
};

struct flush_context {
	int flush_interval;
};

LIST_HEAD(proc_list, active_process);
struct proc_list procs = LIST_HEAD_INITIALIZER(procs);

struct ConfigurationList ConfigList[] = {
	{ "", 					tCOMMENT,	NULL},
	{ "BBS_HOST",			tSTRING,	(int*)&Bbs_Host},
	{ "BBSD_PORT",			tINT,		(int*)&Bbsd_Port },
	{ "BBS_CALL",           tSTRING,    (int*)&Bbs_Call },
	{ "", 					tCOMMENT,	NULL},
	{ "BIDD_PORT",			tINT,		(int*)&Bidd_Port },
	{ "BIDD_BIND_ADDR",		tSTRING,	(int*)&Bidd_Bind_Addr },
	{ "", 					tCOMMENT,	NULL},
	{ "BIDD_FILE",			tFILE,		(int*)&Bidd_File },
	{ "BIDD_FLUSH",			tTIME,		&Bidd_Flush},
	{ "BIDD_AGE",			tTIME,		&Bidd_Age },
	{ NULL, 				tEND,		NULL}};

static void accept_callback(void *ctx, void *arg0, int arg1);
static void service_callback(void *ctx, void *arg0, int arg1);
static void bbsd_activity_callback(void *ctx, void *arg0, int arg1);
static void flush_callback(void *ctx, void *arg0, int arg1);
static void log_clear_callback(void *ctx, void *arg0, int arg1);
static int  handle_line(struct active_process *ap, char *buf);
static struct active_process *add_proc();
static void remove_proc(struct active_process *ap);
static int service_port(struct active_process *ap);

static void
display_config(void)
{
	printf("      Bbs_Host = %s\n", Bbs_Host);
	printf("     Bbsd_Port = %d\n", Bbsd_Port);
	printf("     Bidd_Port = %d\n", Bidd_Port);
	printf("Bidd_Bind_Addr = %s\n", Bidd_Bind_Addr != NULL
		? Bidd_Bind_Addr
		: "(none)");
	printf("     Bidd_File = %s\n", Bidd_File);
	printf("    Bidd_Flush = %d\n", Bidd_Flush);
	printf("      Bidd_Age = %d\n", Bidd_Age);
	fflush(stdout);
	exit(0);
}

static void
accept_callback(void *ctx, void *arg0, int arg1)
{
	struct active_process *ap;
	int fd, res;
	intptr_t listen_sock;
	char buf[80];
	alCallback cb;

	listen_sock = (intptr_t) ctx;
	fd = socket_accept_nonblock_unmanaged(listen_sock);
	if (fd < 0) {
		log_error("accept failed on new connection.");
		return;
	}

	if (socket_write(fd, daemon_version("bidd", Bbs_Call)) != sockOK) {
		log_warning("couldn't write hello banner.");
		close(fd);
		return;
	}

        if ((ap = add_proc()) == NULL) {
		log_error("couldn't allocate process.");
		close(fd);
		return;
	}
        ap->fd = fd;

        AL_CALLBACK(&cb, ap, service_callback);
        res = alEvent_registerFd(fd, ALFD_READ, cb, &ap->ev);
        if (res != 0) {
                log_error("couldn't register new process");
                remove_proc(ap);
        }
}

static void
service_callback(void *ctx, void *arg0, int arg1)
{
	struct active_process *ap = (struct active_process *) ctx;

	if (service_port(ap) == ERROR) {
		remove_proc(ap);
	}
}

static void
bbsd_activity_callback(void *ctx, void *arg0, int arg1)
{
	shutdown_daemon = TRUE;
}

static int
service_port(struct active_process *ap)
{
	int res;
	char *buf;

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
	char *c;

	log_f("bidd", "R:", buf);
	if((c = parse(buf)) == NULL)
		return ERROR;

	log_f("bidd", "S:", c);
	if(socket_raw_write(ap->fd, c) == ERROR)
		return ERROR;

	return OK;
}

static struct active_process *
add_proc()
{
	struct active_process *ap = malloc_struct(active_process);
	if (ap == NULL)
		goto AllocFailed;

	ap->buf = async_line_new(1024, 1 /* filter CR */);
	if (ap->buf == NULL)
		goto AsyncAllocFailed;

	ap->fd = -1;

	LIST_INSERT_HEAD(&procs, ap, entries);

	return ap;

AsyncAllocFailed:
	free(ap);
AllocFailed:
	return NULL;
}

static void
remove_proc(struct active_process *ap)
{
	LIST_REMOVE(ap, entries);

	if (ap->ev != NULL) {
		alEvent_deregister(ap->ev);
	}

	close(ap->fd);
	async_line_free(ap->buf);
	free(ap);
}

int
main(int argc, char *argv[])
{
	intptr_t listen_sock;
	int bbsd_sock, res;
	alEventHandle listen_ev, bbsd_ev;
	alCallback cb;

	bbs_log_init("b_bidd", 1 /* Also log to stderr */);

	parse_options(argc, argv, ConfigList, "BIDD - Bid Daemon");

	if (dbug_level & dbgVERBOSE)
		bbs_log_level(BBS_LOG_DEBUG);
	if(dbug_level & dbgTESTHOST)
		test_host(Bbs_Host);
	if(!(dbug_level & dbgFOREGROUND))
		daemon(1, 1);

	if(bbsd_open(Bbs_Host, Bbsd_Port, "bidd", "DAEMON") == ERROR)
		exit(1);

	bbsd_get_configuration(ConfigList);
	bbsd_msg("Startup");
	bbsd_sock = bbsd_socket();
	time_now = bbsd_get_time();

	if(VERBOSE)
		display_config();

	if(read_new_file(Bidd_File) == ERROR) {
		log_error("Error opening %s", Bidd_File);
		return 1;
	}

	if((listen_sock = socket_listen(Bidd_Bind_Addr, &Bidd_Port)) == ERROR)
		return 1;

	bbsd_port(Bidd_Port);
	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("");

	if (alEvent_init() != 0) {
		log_error("Unable to initialize event system");
		return 1;
	}

	AL_CALLBACK(&cb, NULL, log_clear_callback);
	res = alEvent_addTimer(Log_Clear_Interval_s * 1000, 0, cb);
	if (res < 0) {
		log_error("can't register log clear timer");
		return 1;
	}

	AL_CALLBACK(&cb, NULL, flush_callback);
	res = alEvent_addTimer(Bidd_Flush * 1000, 0, cb);
	if (res < 0) {
		log_error("can't register flush timer");
		return 1;
	}

	AL_CALLBACK(&cb, (void *)listen_sock, accept_callback);
	if (alEvent_registerFd(listen_sock, ALFD_READ, cb, &listen_ev) != 0) {
		log_error("Unable to register listen socket");
		return 1;
	}

	AL_CALLBACK(&cb, NULL, bbsd_activity_callback);
	if (alEvent_registerFd(bbsd_sock, ALFD_READ, cb, &bbsd_ev) != 0) {
		log_error("Unable to register bbsd socket");
		return 1;
	}

	signal(SIGPIPE, SIG_IGN);

	while (!shutdown_daemon && alEvent_pending()) {
		alEvent_poll();
	}

	if(bid_image == DIRTY)
		write_file();

	return 0;
}

static void
flush_callback(void *ctx, void *arg0, int arg1)
{
	int res;
	alCallback cb;

	age();

	if(bid_image == DIRTY)
		write_file();

	AL_CALLBACK(&cb, ctx, flush_callback);
	res = alEvent_addTimer(Log_Clear_Interval_s * 1000, 0, cb);
	if (res < 0) {
		log_error("can't register flush timer");
		shutdown_daemon = TRUE;
	}
}

static void
log_clear_callback(void *ctx, void *arg0, int arg1)
{
	int res;
	alCallback cb;

	log_clear("bidd");

	AL_CALLBACK(&cb, ctx, log_clear_callback);
	res = alEvent_addTimer(Log_Clear_Interval_s * 1000, 0, cb);
	if (res < 0) {
		log_error("can't register log clear timer");
		shutdown_daemon = TRUE;
	}
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
