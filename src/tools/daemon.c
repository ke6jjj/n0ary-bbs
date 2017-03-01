/*	Detach a daemon process from login session context */

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "tools.h"

void
test_host(char *host)
{
	char *dotptr;
	struct hostent *hp;
	char name[80];

	if(gethostname(name, 80) < 0) {
		printf("Error reading host name\n");
		exit(1);
	}

	if ((dotptr = strchr(name,'.')) != NULL)
		*dotptr = 0;

	if((hp = gethostbyname(host)) == NULL) {
		printf("could not find \"%s\" in the hosts database\n", host);
		exit(1);
	}

	if ((dotptr = strchr(hp->h_name,'.')) != NULL)
		*dotptr = 0;

	if (strcmp(hp->h_name, name) != 0) {
		printf("\"%s\" resolves to \"%s\"\n", host, hp->h_name);
		printf("Must be run from \"%s\" which is \"%s\"\n", name, hp->h_name);
	}
}
