#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"

char *Bin_Dir;

struct ConfigurationList ConfigList[] = {
	{ "", 					tCOMMENT,	NULL},
	{ "BBS_HOST",			tSTRING,	(void*)&Bbs_Host},
	{ "BBSD_PORT",			tINT,		(void*)&Bbsd_Port },
	{ "BIN_DIR",            tDIRECTORY, (void*)&Bin_Dir },
	{ NULL, 				tEND,		NULL}};

/* Determining which phone line a user is entering on or whether or
 * not it is the console is difficult and varies between systems.
 * In my case I use a portmaster terminal server which is a really
 * nice device but it actually logs people in via ttyp* devices. This
 * makes it impossible to determine which serial port is actually in
 * use. In simpler setup you can just do a ttyname() and it will
 * tell you the device name.
 * 
 * In my case I have the portmaster change the TERM type which is
 * available via a system call.
 */

struct Names {
	char *txt, *port;
} names[] = {
#if 1
	{ "vt100", "PHONE0" },
	{ "vt220", "PHONE1" },
#else
	{ "/dev/ttyc0", "PHONE0" },
	{ "/dev/ttyc1", "PHONE1" },
#endif
	{ NULL, "CONSOLE" }};


char *
method()
{
	struct Names *n = names;
#if 1
	char *s = (char*)getenv("TERM");
#else
	char *s = (char*)ttyname(0);
#endif
		/* Now scan through the list of possible keys looking for a match.
		 * If none are found then just use the NULL as the default entry.
		 */

	while(n->txt != NULL) {
		if(!strcmp(s, n->txt))
			break;
		n++;
	}
	return n->port;
}

/* This program is called by one of two names, "dialin" and "dialinBS".
 * The name determines what character is to be used for backspace.
 */

/*ARGSUSED*/
int
main(argc, argv)
int argc;
char *argv[];
{
	char pgm[256];
	char call[80], *p;
	int bad;

    if(bbsd_open(Bbs_Host, Bbsd_Port, "DialIn", "STATUS") == ERROR)
		error_print_exit(0);
	error_clear();

	bbsd_get_configuration(ConfigList);
    bbsd_msg("Prompt for call");

#if 1
#ifdef BS
	printf("erase character = ^H (Backspace)\n");
	system("stty erase ");
#else
	printf("erase character = ^? (Delete)\n");
	system("stty erase ");
#endif
#else
	printf("argv[0] = %s\n", argv[0]);
	p = argv[0];
	while(*p) p++;
	p-=2;

	if(!strcmp(p, "BS")) {
		printf("erase character = ^H (Backspace)\n");
		system("stty erase ");
	} else {
		printf("erase character = ^? (Delete)\n");
		system("stty erase ");
	}
#endif

	do {
		p = call;
		do {
			printf("Enter your callsign (first name if non-ham): ");
			safegets(call, sizeof(call));
		} while(*p == 0);

		call[6]=0;

		bad = 0;
		while(*p) {
			if(!isalnum(*p)) {
				printf("\nError in callsign only letters and numbers are allowed\n");
				printf("please try again.\n");
				bad = 1;

				if(*p == '') {
					printf("changing erase character = ^H (Backspace)\n");
					system("stty erase ");
				}
				if(*p == '') {
					printf("changing erase character = ^? (Delete)\n");
					system("stty erase ");
				}
			}
			p++;
		}
	} while(bad);

	bbsd_close();

	sprintf(pgm, "%s/b_bbs", Bin_Dir);
	execl(pgm, "b_bbs", "-v", method(), call, 0);

	return 0;
}
