#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"

int Bbsd_Port = BBSD_PORT;
char *Bbs_Host = BBS_HOST;
int Logging = logOFF;
int dbug_level = dbgNONE;
char *config_fn = NULL;
int ReceiveSocket = 0;
int SendSocket = 0;

static void
usage(char *pgm)
{
	printf("usage:\n");
	printf("\t%s [-w|W] [-f filename] [-l|L] [-d#] [-h host] [-p port]\n", pgm);
	printf("\t\t-w|W\tDisplay required configurtion variables\n");
	printf("\t\t-f\tWrite configuration variables to \"filename\"\n");
	printf("\t\t-l|L\tEnable logging, (l=continues, L=clears when idle\n");
	printf("\t\t-d\tEnable debugging (# is hex value)\n");
	printf("\t\t\t\t0x001\tverbose\n");
	printf("\t\t\t\t0x100\trun program in foreground\n");
	printf("\t\t\t\t0x200\tdon't test for hostname (obsolete, now default)\n");
	printf("\t\t\t\t0x400\tdon't connect to other daemons\n");
	printf("\t\t\t\t0x800\tdon't send mail\n");
	printf("\t\t\t\t0x1000\ttest for hostname\n");
	printf("\n");
	exit(1);
}

void
parse_options(int argc, char *argv[],  struct ConfigurationList *cl, char *me)
{
	int c;
	int show_config = 0;
	extern char *optarg;

	while((c = getopt(argc, argv, "S:R:p:lLwWd:f:h:?")) != -1) {
		char *p = optarg;
		switch(c) {
        case 'W':
			show_config++;
		case 'w':
			show_config++;
			break;
		case 'f':
			config_fn = optarg;
			break;
		case 'p':
			Bbsd_Port = atoi(optarg);
			break;
		case 'S':
			SendSocket = atoi(optarg);
			break;
		case 'R':
			ReceiveSocket = atoi(optarg);
			break;
		case 'h':
			Bbs_Host = optarg;
			break;

		case 'd':
			dbug_level = get_hexnum(&p);
			break;
		case 'l':
			Logging = logON;
			break;
		case 'L':
			Logging = logONnCLR;
			break;
		case '?':
			usage(argv[0]);
			exit(0);
		}
	}

	/* Test for nonsense options */
	if ((dbug_level & dbgTESTHOST) && (dbug_level & dbgIGNOREHOST)) {
		fprintf(stderr, "Conflicting host options; can't both test and ignore host.\n");
		exit(1);
	}

	if(show_config) {
		if(show_config > 1)
			show_configuration_rules(config_fn);
		show_reqd_configuration(cl, me, config_fn);
		exit(0);
	}
}
