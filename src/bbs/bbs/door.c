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
static int
door(const char *name, const char *path)
{
	int res, fd_in, fd_out;
	ExtSession ls;
	const char *argv[1], *envk[3], *envv[3];
	char lines[10];

	snprintf(lines, sizeof(lines), "%ld", Lines);

	argv[0] = name;

	envk[0] = "BBS_DIR";                 envv[0] = Bbs_Dir;
	envk[1] = "BBS_USER";                envv[1] = usercall;
	envk[2] = "BBS_USER_LINES";          envv[2] = lines;

	res = ExtSession_init(&ls, DoCRLFEndings,
		path,
		1, argv,
		3, envk, envv);

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

int
adventure(void)
{
	return door("adventure", "/usr/home/bbs/doors/bin/adventure");
}

int
zork(void)
{
	return door("zork", "/usr/home/bbs/doors/bin/zork");
}
