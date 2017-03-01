#include <stdio.h>
#include <fcntl.h>

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

void
parse_options(int argc, char *argv[])
{
	int i;
	struct PortDefinition *pd = port_table();

	if(argc == 2)
		while(pd->port != 0) {
			if(!strcmp(argv[1], pd->name)) {
				monitor = tnc_monitor(pd->name);
				Tncd_Name = pd->name;
				Tncd_Host = tnc_host(pd->name);
				return;
			}
			pd++;
		}
	usage(argv[0]);
}

main(int argc, char *argv[])
{
	int cnt, fd;
	char buf[1025], filename[1025];

	parse_options(argc, argv);

	if((log_sock = open_socket(Tncd_Host, monitor)) == ERROR) {
		printf("Couldn't attach to port %s:%d\n", Tncd_Host, monitor);
		exit(1);
	}

	sprintf(filename, "%s/%s.log", DIR, Tncd_Name);
	if((fd = open(filename, O_WRONLY|O_CREAT, 0x3FF)) < 0) {
		perror(filename);
		exit(1);
	}

	while(cnt = read(log_sock, buf, 1024)) {
		write(fd, buf, cnt);
	}
	close(fd);
}
