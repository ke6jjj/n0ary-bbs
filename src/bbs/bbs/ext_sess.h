/*
 * External session handling.
 *
 * Allows a BBS user to interact with any program that expects a TTY-like
 * interface on stdin, stdout and stderr.
 */
#ifndef BBS_EXT_SESS_H
#define BBS_EXT_SESS_H

#include <sys/wait.h>
#include <unistd.h>

#include "alib.h"

enum ExtSession_Error {
	EXTSESS_OK = 0,
	EXTSESS_UNKNOWN,
	EXTSESS_MEMORY,
	EXTSESS_OPEN_PTY,
	EXTSESS_PTY_NONBLOCK,
	EXTSESS_PTS_NAME,
	EXTSESS_AL_EVENT_INIT,
	EXTSESS_REGISTER_STDIN,
	EXTSESS_REGISTER_SLAVE,
	EXTSESS_FORK,
	EXTSESS_REGISTER_CHILD,
	EXTSESS_CHILD_EXEC,
	EXTSESS_CHILD_SLAVE_STDOUT,
	EXTSESS_CHILD_SLAVE_STDIN,
	EXTSESS_CHILD_DUP_STDOUT,
	EXTSESS_CHILD_DUP_STDIN,
	EXTSESS_CHILD_DUP_STDERR,
	EXTSESS_CHILD_SETSID,
	EXTSESS_CHILD_CTTY,
};
typedef enum ExtSession_Error ExtSession_Error;

struct ExtSession {
	char buf[1024];
	char *prog;
	char **argv;
	char **env_vars;
	char **env_values;
	int argc;
	int envc;
	int master_fd;
	int stdin_fd;
	int stdout_fd;
	char *slave_path;
	pid_t childPid;
	int childExited;
	int childExitCode;
	int childWaitTimer;
	int run;
	int translate_crlf;
	alEventHandle stdinHandle;
	alEventHandle slaveHandle;
	alEventHandle childHandle;
};
typedef struct ExtSession ExtSession;

int ExtSession_init(ExtSession *ls, int translate_crlf, const char *prog,
	int argc, const char **argv, int envc,
	const char **env_vars, const char **env_values);
int ExtSession_run(ExtSession *ls, int fd_in, int fd_out);
void ExtSession_deinit(ExtSession *ls);
const char *ExtSession_error(ExtSession_Error);

#endif
