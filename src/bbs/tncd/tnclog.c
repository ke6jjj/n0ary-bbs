#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"

#define DIR	BBS_DIR"/LOG"

#define TNC0	1
#define TNC1	2
#define TNC2	4
#define TNC3	8

static int log_sock = ERROR;
static int monitor = 0;
char *Tncd_Host;
char *Tncd_Name;
 
void
usage(char *name)
{
	fprintf(stderr, "Usage:\n\t%s TNCx\n", name);
	exit(1);
}

static void
parse_tnclog_options(int argc, char *argv[])
{
	int i;
	struct PortDefinition *pd = port_table();

	if(argc == 2)
		while(pd != NULL) {
			if(!strcmp(argv[1], pd->name)) {
				monitor = tnc_monitor_port(pd->name);
				Tncd_Name = pd->name;
				Tncd_Host = tnc_host(pd->name);
				return;
			}
			pd = pd->next;
		}
	usage(argv[0]);
}

int
main(int argc, char *argv[])
{
	int cnt, fd;
	char buf[1025], filename[1025];

	parse_tnclog_options(argc, argv);

	if((log_sock = socket_open(Tncd_Host, monitor)) == ERROR) {
		printf("Couldn't attach to port %s:%d\n", Tncd_Host, monitor);
		return 1;
	}

	sprintf(filename, "%s/%s.log", DIR, Tncd_Name);
	if((fd = open(filename, O_WRONLY|O_CREAT, 0x3FF)) < 0) {
		perror(filename);
		return 1;
	}

	while(cnt = read(log_sock, buf, 1024)) {
		write(fd, buf, cnt);
	}
	close(fd);

	return 0;
}
