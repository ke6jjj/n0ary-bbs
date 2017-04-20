#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bbslib.h"
#include "function.h"
#include "tokens.h"
#include "c_cmmn.h"
#include "ext_sess.h"
#include "vars.h"

extern int sock;

/*
 * Spawn /usr/bin/login in a new psuedo-terminal and connect the I/O
 * from the pseudo-terminal to the BBS user. During this time all monitoring
 * activity and BBSD servicing will be put on hold.
 */
int
unixlogin(void)
{
	int res, fd_in, fd_out;
	ExtSession ls;
        struct TOKEN *t = TokenList;
	char **argv;

        NEXT(t);

	if(t->token == END) {
		PRINTF("Need user name.\n");
		return OK;
	}

	argv = malloc(2 * sizeof(char *));
	if (argv == NULL) {
		PRINTF("Out of memory.\n");
		return OK;
	}

	argv[0] = "login";
	argv[1] = t->lexem;

	res = ExtSession_init(&ls, DoCRLFEndings, "/usr/bin/login", 2, argv);
	free(argv);

	if (res != EXTSESS_OK) {
		PRINTF("Error: %s\n", ExtSession_error(res));
		return OK;
	}

	if (sock == ERROR) {
		fd_in = STDIN_FILENO;
		fd_out = STDOUT_FILENO;
	} else {
		fd_in = sock;
		fd_out = sock;
	}

	res = ExtSession_run(&ls, fd_in, fd_out);

	ExtSession_deinit(&ls);

	if (res != EXTSESS_OK)
		PRINTF("Error: %s\n", ExtSession_error(res));

	return OK;
}
