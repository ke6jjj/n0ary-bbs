#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>

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

static int
service_port(struct active_processes *ap)
{
    char *c, *s, buf[1024];

    c = buf;
    while(TRUE) {
        if(read(ap->fd, c, 1) <= 0)
            return ERROR;
		if(*c == '\r')
			continue;

        if(*c == '\n') {
			*c = 0;
            break;
		}
		c++;
    }

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

	snprintf(buf, sizeof(buf),
		"LOGIN %d UnKwn STATUS %ld", ap->proc_num, ap->t0);
	textline_append(&Notify, buf);

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
	textline_append(&Notify, buf);

	daemon_check_out(ap);

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

/* Every process must checkin every KILL_INTERVAL seconds. If not
 * then the process should be killed if it has supplied us with a 
 * PID.
 */
static void
chk_procs(void)
{
	struct active_processes *ap = procs;
	long threshold = time(NULL) - KILL_INTERVAL;

	while(ap) {
		if(ap->pid != ERROR)
			if(ap->idle < threshold)
				kill(ap->pid, SIGKILL);
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
		Bbs_Host = s;

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
	int listen_sock;
	int ping_interval = PING_INTERVAL;
	struct active_processes *ap;
	int dmnchk = TRUE;

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
	while(TRUE) {
		int cnt;
		fd_set ready;
		int fdlimit;
		struct timeval t;

		if(shutdown_daemon)
			return 0;

		ap = procs;
		FD_ZERO(&ready);
		FD_SET(listen_sock, &ready);
		fdlimit = listen_sock;
		
		while(ap) {
			if(ap->fd != ERROR) {
				FD_SET(ap->fd, &ready);
				if(ap->fd > fdlimit)
					fdlimit = ap->fd;
			}
			NEXT(ap);
		}
		fdlimit++;

			/* if we have statuses pending then make the timeout short
			 * this allows us to buffer up a little if it comes in hunks.
			 */

		t.tv_usec = 0;
		if(Notify || dmnchk)
			t.tv_sec = 2;
		else
			t.tv_sec = ping_interval;

		if((cnt = select(fdlimit, &ready, NULL, NULL, &t)) < 0)
			continue;
		
		if(cnt == 0) {
			if(Notify != NULL)
				notify_monitors();
			else {
				if(ping_interval < 120)
					ping_interval++;
				ping_monitors();
			}
#if 0
			chk_procs();
#endif
			dmnchk = daemons_start();
			continue;
		}

		ping_interval = PING_INTERVAL;

		if(FD_ISSET(listen_sock, &ready)) {
			ap = add_proc();
			if((ap->fd = socket_accept(listen_sock)) < 0)
				ap = remove_proc(ap);
			else {
				if(socket_write(ap->fd, daemon_version("bbsd", Bbs_Call)) != sockOK)
					ap = remove_proc(ap);
				else {
					char buf[80];
					snprintf(buf, sizeof(buf),
						"# %d\n", ap->proc_num);
					if(write(ap->fd, buf, strlen(buf)) < 0)
						ap = remove_proc(ap);
				}
			}
		}

		ap = procs;
		while(ap) {
			if(FD_ISSET(ap->fd, &ready)) {
				if(service_port(ap) == ERROR) {
					ap = remove_proc(ap);
					continue;
				}
			}
			NEXT(ap);
		}

		if(Notify != NULL)
			notify_monitors();
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
