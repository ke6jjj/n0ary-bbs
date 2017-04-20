#include <stdlib.h>

#include "bbslib.h"
#include "c_cmmn.h"
#include "door.h"
#include "ext_sess.h"
#include "function.h"
#include "user.h"
#include "vars.h"

extern int sock;

/*
 * Spawn a door in a new psuedo-terminal and connect the I/O
 * from the pseudo-terminal to the BBS user. During this time all monitoring
 * activity and BBSD servicing will be put on hold.
 */
int
adventure(void)
{
	int res, fd_in, fd_out;
	ExtSession ls;
	char **argv;

	argv = malloc(sizeof(char *) * 3);
	if (argv == NULL) {
		PRINT("Out of memory.");
		return ERROR;
	}

	argv[0] = "adventure";
	argv[1] = Bbs_Dir;
	argv[2] = usercall;
	res = ExtSession_init(&ls, DoCRLFEndings,
		"/usr/home/bbs/doors/bin/adventure", 3, argv);
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
