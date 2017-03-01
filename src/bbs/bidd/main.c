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

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bid.h"

time_t
	time_now = 0,
	Bidd_Age,
	Bidd_Flush;

int
	Bidd_Port;

char
    *Bbs_Call,
	*Bidd_File;

char output[4096];
int shutdown_daemon = FALSE;

struct active_processes {
	struct active_processes *next;
	int fd;
} *procs = NULL;

struct ConfigurationList ConfigList[] = {
	{ "", 					tCOMMENT,	NULL},
	{ "BBS_HOST",			tSTRING,	(int*)&Bbs_Host},
	{ "BBSD_PORT",			tINT,		(int*)&Bbsd_Port },
	{ "BBS_CALL",           tSTRING,    (int*)&Bbs_Call },
	{ "", 					tCOMMENT,	NULL},
	{ "BIDD_PORT",			tINT,		(int*)&Bidd_Port },
	{ "", 					tCOMMENT,	NULL},
	{ "BIDD_FILE",			tFILE,		(int*)&Bidd_File },
	{ "BIDD_FLUSH",			tTIME,		(int*)&Bidd_Flush},
	{ "BIDD_AGE",			tTIME,		(int*)&Bidd_Age },
	{ NULL, 				tEND,		NULL}};

static void
display_config(void)
{
	printf("  Bbs_Host = %s\n", Bbs_Host);
	printf(" Bbsd_Port = %d\n", Bbsd_Port);
	printf(" Bidd_Port = %d\n", Bidd_Port);
	printf(" Bidd_File = %s\n", Bidd_File);
	printf("Bidd_Flush = %ld\n", Bidd_Flush);
	printf("  Bidd_Age = %ld\n", Bidd_Age);
	fflush(stdout);
	exit(0);
}

int
service_port(struct active_processes *ap)
{
    char *c, buf[256];

	if(socket_read_line(ap->fd, buf, 256, 10) == ERROR)
		return ERROR;

	log_f("bidd", "R:", buf);
	if((c = parse(buf)) == NULL)
		return ERROR;

	log_f("bidd", "S:", c);
	if(socket_raw_write(ap->fd, c) == ERROR)
		return ERROR;

	return OK;
}

struct active_processes *
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
    return ap;
}

struct active_processes *
remove_proc(struct active_processes *ap)
{
    struct active_processes *tap = procs;

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

    socket_close(ap->fd);
    free(ap);
    return tap;
}

int
main(int argc, char *argv[])
{
	int listen_sock;
	int bbsd_sock;
	struct timeval t;
	struct active_processes *ap;
	time_t flush_time = Time(NULL) + Bidd_Flush;

	parse_options(argc, argv, ConfigList, "BIDD - Bid Daemon");

	if(!(dbug_level & dbgIGNOREHOST))
		test_host(Bbs_Host);
	if(!(dbug_level & dbgFOREGROUND))
		daemon(1, 1);

    if(bbsd_open(Bbs_Host, Bbsd_Port, "bidd", "DAEMON") == ERROR)
		error_print_exit(0);
	error_clear();

	bbsd_get_configuration(ConfigList);
    bbsd_msg("Startup");
	bbsd_sock = bbsd_socket();
	time_now = bbsd_get_time();

	if(VERBOSE)
		display_config();

	if(read_new_file(Bidd_File) == ERROR) {
		printf("Error opening %s\n", Bidd_File);
		return 1;
	}

	if((listen_sock = socket_listen(&Bidd_Port)) == ERROR)
		return 1;

    bbsd_port(Bidd_Port);
	if(!(dbug_level & dbgNODAEMONS))
		bbsd_msg("");

	signal(SIGPIPE, SIG_IGN);

	while(TRUE) {
		int cnt;
		fd_set ready;
		int fdlimit;

		error_print();

		if(shutdown_daemon) {
			if(bid_image == DIRTY)
				write_file();
			return 0;
		}

		ap = procs;
		FD_ZERO(&ready);
		FD_SET(listen_sock, &ready);
		fdlimit = listen_sock;

		FD_SET(bbsd_sock, &ready);
		if(bbsd_sock > fdlimit)
			fdlimit = bbsd_sock;

		while(ap) {
			if(ap->fd != ERROR) {
				FD_SET(ap->fd, &ready);
				if(ap->fd > fdlimit)
					fdlimit = ap->fd;
			}
			NEXT(ap);
		}
		fdlimit += 1;

		t.tv_sec = 60;
		t.tv_usec = 0;

		if((cnt = select(fdlimit, (void*)&ready, NULL, NULL, &t)) < 0)
			continue;
		
		if(cnt == 0) {
			time_t now = Time(NULL);
			log_clear("bidd");
			if(now > flush_time) {
				flush_time = now + Bidd_Flush;

				age();

				if(bid_image == DIRTY)
					write_file();
			}
			continue;
		}

		if(FD_ISSET(bbsd_sock, &ready))
			shutdown_daemon = TRUE;

        if(FD_ISSET(listen_sock, &ready)) {
            ap = add_proc();
            if((ap->fd = socket_accept(listen_sock)) < 0)
                ap = remove_proc(ap);
			else
				if(socket_write(ap->fd,	daemon_version("bidd", Bbs_Call)) == ERROR)
					ap = remove_proc(ap);
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
