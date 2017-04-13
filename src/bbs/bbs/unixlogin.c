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

#include "alib.h"
#include "bbslib.h"
#include "function.h"
#include "tokens.h"
#include "c_cmmn.h"

extern int sock;

#define err_if(expr, value, var, label) { \
	if ((expr)) { \
		var = value; \
		goto label; \
	} \
}

enum LoginSession_Error {
	LOGIN_OK = 0,
	LOGIN_UNKNOWN,
	LOGIN_MEMORY,
	LOGIN_OPEN_PTY,
	LOGIN_PTY_NONBLOCK,
	LOGIN_PTS_NAME,
	LOGIN_AL_EVENT_INIT,
	LOGIN_REGISTER_STDIN,
	LOGIN_REGISTER_SLAVE,
	LOGIN_FORK,
	LOGIN_REGISTER_CHILD,
	LOGIN_CHILD_EXEC,
	LOGIN_CHILD_SLAVE_STDOUT,
	LOGIN_CHILD_SLAVE_STDIN,
	LOGIN_CHILD_DUP_STDOUT,
	LOGIN_CHILD_DUP_STDIN,
	LOGIN_CHILD_DUP_STDERR,
	LOGIN_CHILD_SETSID,
	LOGIN_CHILD_CTTY,
};
typedef enum LoginSession_Error LoginSession_Error;

struct LoginSession {
	char buf[1024];
	int master_fd;
	int stdin_fd;
	char *slave_path;
	char *username;
	pid_t childPid;
	int childExited;
	int childExitCode;
	int run;
	alEventHandle stdinHandle;
	alEventHandle slaveHandle;
	alEventHandle childHandle;
};
typedef struct LoginSession LoginSession;

static int LoginSession_init(LoginSession *ls, const char *username);
static int LoginSession_run(LoginSession *ls);
static void LoginSession_stop(LoginSession *ls);
static void LoginSession_deinit(LoginSession *ls);
static void LoginSession_stdinRead(void *, void *, int);
static void LoginSession_slaveRead(void *, void *, int);
static void LoginSession_childExit(void *, void *, int);
static void LoginSession_execLogin(LoginSession *);
static const char *LoginSession_error(LoginSession_Error);

/*
 * Spawn /usr/bin/login in a new psuedo-terminal and connect the I/O
 * from the pseudo-terminal to the BBS user. During this time all monitoring
 * activity and BBSD servicing will be put on hold.
 */
int
unixlogin(void)
{
	int res;
	LoginSession ls;
        struct TOKEN *t = TokenList;

        NEXT(t);

	if(t->token == END) {
		PRINTF("Need user name.\n");
		return OK;
	}

	res = LoginSession_init(&ls, t->lexem);
	if (res != LOGIN_OK) {
		PRINTF("Error: %s\n", LoginSession_error(res));
		return OK;
	}

	res = LoginSession_run(&ls);

	LoginSession_deinit(&ls);

	if (res != LOGIN_OK)
		PRINTF("Error: %s\n", LoginSession_error(res));

	return OK;
}

static int
LoginSession_init(LoginSession *ls, const char *username)
{
	int master_fd, res;
	char *username_copy;
	char *slave, *slave_copy;

	username_copy = strdup(username);
	err_if(username_copy == NULL, LOGIN_MEMORY, res, UsernameMallocFailed);

	master_fd = posix_openpt(O_RDWR|O_NOCTTY|O_CLOEXEC);
	err_if(master_fd == -1, LOGIN_OPEN_PTY, res, PtyMasterOpenFailed);

	res = fcntl(master_fd, F_SETFL, O_NONBLOCK);
	err_if(res < 0, LOGIN_PTY_NONBLOCK, res, PtyNonblockFailed);

	slave = ptsname(master_fd);
	err_if(slave == NULL, LOGIN_PTS_NAME, res, PtySlaveGetNameFailed);

	slave_copy = strdup(slave);
	err_if(slave_copy == NULL, LOGIN_MEMORY, res, SlaveMallocFailed);

	ls->username = username_copy;
	ls->master_fd = master_fd;
	ls->slave_path = slave_copy;

	return LOGIN_OK;

SlaveMallocFailed:
PtySlaveGetNameFailed:
PtyNonblockFailed:
	close(master_fd);
PtyMasterOpenFailed:
	free(username_copy);
UsernameMallocFailed:	
	return res;
}

static int
LoginSession_run(LoginSession *ls)
{
	int res, stdin_fd;
	alCallback cb;
	pid_t pid;

	if (sock != ERROR)
		stdin_fd = sock;
	else
		stdin_fd = STDIN_FILENO;

	res = alEvent_init();
	err_if(res < 0, LOGIN_AL_EVENT_INIT, res, AlEventInitFailed);

	/* Get callbacks for data available on BBS user data socket */
	AL_CALLBACK(&cb, ls, LoginSession_stdinRead);
	res = alEvent_registerFd(stdin_fd, ALFD_READ, cb, &ls->stdinHandle);
	err_if(res < 0, LOGIN_REGISTER_STDIN, res, RegisterStdinReaderFailed);

	ls->stdin_fd = stdin_fd;

	/* Get callbacks from data available from login process (and shell */
	AL_CALLBACK(&cb, ls, LoginSession_slaveRead);
	res = alEvent_registerFd(ls->master_fd, ALFD_READ, cb,
		&ls->slaveHandle);
	err_if(res < 0, LOGIN_REGISTER_SLAVE, res, RegisterSlaveReaderFailed);

	/* Fork */
	pid = fork();
	err_if(pid < 0, LOGIN_FORK, res, ForkFailed);

	if (pid == 0) {
		/* child */
		LoginSession_execLogin(ls);
	}

	ls->childPid = pid;
	ls->childExitCode = -1;

	AL_CALLBACK(&cb, ls, LoginSession_childExit);
	res = alEvent_registerProc(pid, ALPROC_EXIT, cb, &ls->childHandle);
	err_if(res < 0, LOGIN_REGISTER_CHILD, res, RegisterChildPidFailed);

	ls->run = 1;

	while (ls->run && alEvent_pending())
		alEvent_poll();

	if (ls->childPid != 0) {
		int dummy;
		kill(ls->childPid, SIGHUP);
		waitpid(ls->childPid, &dummy, 0);
		res = LOGIN_UNKNOWN;
	} else if (ls->childExitCode == -1) {
		res = LOGIN_UNKNOWN;
	} else if (ls->childExitCode != 0) {
		res = ls->childExitCode;
	} else {
		res = LOGIN_OK;
	}

	alEvent_deregister(ls->childHandle);
RegisterChildPidFailed:
ForkFailed:
	alEvent_deregister(ls->slaveHandle);
RegisterSlaveReaderFailed:
	alEvent_deregister(ls->stdinHandle);
RegisterStdinReaderFailed:
	alEvent_shutdown();
AlEventInitFailed:
	return res;
}

void
LoginSession_execLogin(LoginSession *ls)
{
	close(ls->master_fd);

	int my_stdout = open(ls->slave_path, O_WRONLY);
	if (my_stdout < 0)
		exit(LOGIN_CHILD_SLAVE_STDOUT);
	int my_stdin = open(ls->slave_path, O_RDONLY);
	if (my_stdin < 0)
		exit(LOGIN_CHILD_SLAVE_STDIN);
	int res = dup2(my_stdout, STDOUT_FILENO);
	if (res < 0)
		exit(LOGIN_CHILD_DUP_STDOUT);
	res = dup2(my_stdin, STDIN_FILENO);
	if (res < 0)
		exit(LOGIN_CHILD_DUP_STDIN);
	res = dup2(my_stdout, STDERR_FILENO);
	if (res < 0)
		exit(LOGIN_CHILD_DUP_STDERR);
	close(my_stdout);
	close(my_stdin);
	res = setsid();
	if (res < 0)
		exit(LOGIN_CHILD_SETSID);
	res = ioctl(STDIN_FILENO, TIOCSCTTY, NULL);
	if (res < 0)
		exit(LOGIN_CHILD_CTTY);
		
	execl("/usr/bin/login", "login", "jeremyopie", NULL);

	/* exec failure */
	exit(LOGIN_CHILD_EXEC);
}

static void
LoginSession_deinit(LoginSession *ls)
{
	close(ls->master_fd);
	free(ls->slave_path);
	free(ls->username);
}

/* Cause the event loop to stop */
static void
LoginSession_stop(LoginSession *ls)
{
	ls->run = 0;
}

static void
LoginSession_stdinRead(void *lsp, void *arg0, int arg1)
{
	LoginSession *ls = lsp;
	ssize_t nread, nwritten;

	assert(ls->stdinHandle != NULL);

	do {
		nread = read(ls->stdin_fd, ls->buf, sizeof(ls->buf));
		if (nread < 0 && errno != EINTR) {
			PRINTF("stdin read: %s\n", strerror(errno));
			LoginSession_stop(ls);
			return;
		} else if (nread == 0) {
			LoginSession_stop(ls);
			return;
		}
	} while (nread <= 0);

	do {
		nwritten = write(ls->master_fd, ls->buf, nread);
		if (nwritten < 0 && errno != EINTR) {
			PRINTF("stdin copy: %s\n", strerror(errno));
			LoginSession_stop(ls);
			return;
		} else if (nwritten == 0) {
			LoginSession_stop(ls);
			return;
		}
	} while (nwritten <= 0);
}

static void
LoginSession_slaveRead(void *lsp, void *arg0, int arg1)
{
	LoginSession *ls = lsp;
	ssize_t nread;

	assert(ls->slaveHandle != NULL);

	do {
		nread = read(ls->master_fd, ls->buf, sizeof(ls->buf));
		if (nread < 0 && errno != EINTR) {
			PRINTF("slave read: %s\n", strerror(errno));
			LoginSession_stop(ls);
			return;
		} else if (nread == 0) {
			LoginSession_stop(ls);
			return;
		}
	} while (nread <= 0);

	user_write(ls->buf, nread);
}

static void
LoginSession_childExit(void *lsp, void *arg0, int arg1)
{
	LoginSession *ls = lsp;
	int res, code;

	assert(ls->childHandle != NULL);
	assert(ls->childPid > 0);

	do {
		res = waitpid(ls->childPid, &code, WNOHANG);
		if (res < 0 && errno != EINTR) {
			PRINTF("child wait: %s\n", strerror(errno));
			LoginSession_stop(ls);
			return;
		}
	} while (res < 0);

	if (WIFEXITED(code))
		ls->childExitCode = WEXITSTATUS(code);

	ls->childPid = 0;

	LoginSession_stop(ls);
}

static const char *
LoginSession_error(LoginSession_Error err)
{
	switch (err) {
	case LOGIN_OK: return "None";
	case LOGIN_MEMORY: return "Out of memory";
	case LOGIN_OPEN_PTY: return "Open pty";
	case LOGIN_PTY_NONBLOCK: return "Pty nonblock";
	case LOGIN_PTS_NAME: return "ptsname";
	case LOGIN_AL_EVENT_INIT: return "alevent init";
	case LOGIN_REGISTER_STDIN: return "register stdin";
	case LOGIN_REGISTER_SLAVE: return "register slave";
	case LOGIN_FORK: return "fork";
	case LOGIN_REGISTER_CHILD: return "register child";
	case LOGIN_CHILD_EXEC: return "exec";
	case LOGIN_CHILD_SLAVE_STDOUT: return "child stdout";
	case LOGIN_CHILD_SLAVE_STDIN: return "child stdin";
	case LOGIN_CHILD_DUP_STDOUT: return "child dup stdout";
	case LOGIN_CHILD_DUP_STDIN: return "child dup stdin";
	case LOGIN_CHILD_DUP_STDERR: return "child dup stderr";
	case LOGIN_CHILD_SETSID: return "child setsid";
	case LOGIN_CHILD_CTTY: return "child CTTY";
	case LOGIN_UNKNOWN:
	default:
		return "?";
	}
}
