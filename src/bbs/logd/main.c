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

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "log.h"

int
	shutdown_daemon = FALSE,
	Logd_Port;

char
    *Bbs_Call,
	*Logd_Dir,
	*Logd_File;

struct processes *procs = NULL;

struct ConfigurationList ConfigList[] = {
	{ "",					tCOMMENT,	NULL },
	{ "BBS_HOST",			tSTRING,	(int*)&Bbs_Host },
	{ "BBSD_PORT",			tINT,		(int*)&Bbsd_Port },
	{ "BBS_CALL",			tSTRING,	(int*)&Bbs_Call },
	{ "",					tCOMMENT,	NULL },
	{ "LOGD_PORT",			tINT,		(int*)&Logd_Port },
	{ "LOGD_FILE",			tFILE,		(int*)&Logd_File },
	{ "LOGD_DIR",			tDIRECTORY,	(int*)&Logd_Dir },
	{ NULL, 0, NULL}};

static int
service_port(struct processes *ap)
{
    char buf[4096];

	if(socket_read_line(ap->fd, buf, 4096, 10) == ERROR)
		return ERROR;

	if(ap->whoami[0] == 0)
		strcpy(ap->whoami, buf);
	else
		textline_append(&(ap->txt), buf);
	return OK;
}

static struct processes *
add_proc()
{
    struct processes
        *tap = procs,
        *ap = malloc_struct(processes);

    if(tap == NULL)
        procs = ap;
    else {
        while(tap->next)
            NEXT(tap);
        tap->next = ap;
    }

	ap->txt = NULL;
	ap->t0 = time(NULL);
	ap->fd = ERROR;
    return ap;
}

static void
write_file(struct processes *ap)
{
	struct text_line *tl = ap->txt;
	struct tm *tm;
	char dbuf[80];

	FILE *fp = fopen(Logd_File, "a+");
	if(fp == NULL)
		return;

	fprintf(fp, "=== %s\t", ap->whoami);

	tm = localtime(&(ap->t0));
	strftime(dbuf, 80, "%y%m%d:%H""%M", tm);
	fprintf(fp, "%s ", dbuf);

	tm = localtime(&(ap->t1));
	strftime(dbuf, 80, "%y%m%d:%H""%M", tm);
	fprintf(fp, "%s\n", dbuf);

	while(tl) {
		fprintf(fp, "  %s\n", tl->s);
		NEXT(tl);
	}
	fprintf(fp, "\n");
	fclose(fp);
}

static struct processes *
remove_proc(struct processes *ap)
{
    struct processes *tap = procs;
	ap->t1 = time(NULL);

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

	write_file(ap);
    socket_close(ap->fd);
    free(ap);
    return tap;
}


void
write_config_file(void)
{
	printf("HOST %s\n", Bbs_Host);
	printf("PORT %d\n", Logd_Port);
	printf("LOGFILE %s\n", Logd_File);
}

void
read_config_file(char *fn)
{
	FILE *fp = fopen(fn, "r");
	char buf[80];

	if(fp == NULL) {
		printf("Could not open configuration file %s\n", fn);
		return;
	}

	while(fgets(buf, 80, fp)) {
		char *s = buf;
		char *field = get_string(&s);

		if(!strcmp(field, "LOGFILE")) {
			char *val = get_string(&s);
			Logd_File = (char *)malloc(strlen(val)+1);
			strcpy(Logd_File, val);
			printf("LOGFILE = %s\n", Logd_File);
			continue;
		}
		if(!strcmp(field, "HOST")) {
			char *val = get_string(&s);
			Bbs_Host = (char *)malloc(strlen(val)+1);
			strcpy(Bbs_Host, val);
			printf("HOST = %s\n", Bbs_Host);
			continue;
		}
		if(!strcmp(field, "PORT")) {
			Logd_Port = get_number(&s);
			printf("PORT = %d\n", Logd_Port);
			continue;
		}
	}
	fclose(fp);
}

main(int argc, char *argv[])
{
	int listen_port;
	int listen_sock;
	int bbsd_sock;
	struct processes *ap;

	parse_options(argc, argv, ConfigList, "LOGD - Log Daemon");

	if(!(dbug_level & dbgIGNOREHOST))
		test_host(Bbs_Host);
	if(!(dbug_level & dbgFOREGROUND))
		daemon();

	if(bbsd_open(Bbs_Host, Bbsd_Port, "logd", "DAEMON") == ERROR)
		error_print_exit(0);
	error_clear();

	bbsd_get_configuration(ConfigList);
	bbsd_sock = bbsd_socket();

	listen_port = Logd_Port;
	if((listen_sock = socket_listen(&listen_port)) == ERROR)
		exit(1);

	bbsd_port(Logd_Port);
	signal(SIGPIPE, SIG_IGN);

	while(TRUE) {
		fd_set ready;
		int fdlimit;

		if(shutdown_daemon == TRUE)
			exit(0);

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

		if(select(fdlimit, &ready, NULL, NULL, NULL) < 0)
			continue;
		
		if(FD_ISSET(bbsd_sock, &ready))
			shutdown_daemon = TRUE;

		if(FD_ISSET(listen_sock, &ready)) {
			ap = add_proc();
			if((ap->fd = socket_accept(listen_sock)) < 0)
				ap = remove_proc(ap);
			else
				if(socket_write(ap->fd, daemon_version("logd", Bbs_Call)) < 0)
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
}
