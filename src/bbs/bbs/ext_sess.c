/*
 * External session handling.
 *
 * Allows a BBS user to interact with any program that expects a TTY-like
 * interface on stdin, stdout and stderr.
 */
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

#include "ext_sess.h"

#include "alib.h"
#include "bbslib.h"
#include "function.h"
#include "c_cmmn.h"

#define err_if(expr, value, var, label) { \
	if ((expr)) { \
		var = value; \
		goto label; \
	} \
}

static void ExtSession_stdinRead(void *, void *, int);
static void ExtSession_slaveRead(void *, void *, int);
static void ExtSession_childDone(ExtSession *ls, int code);
static void ExtSession_childExit(void *, void *, int);
static void ExtSession_childWaitTimeout(void *, void *, int);
static void ExtSession_execChild(ExtSession *ls);
static void ExtSession_stop(ExtSession *ls);

static int strvec_copy(size_t count, const char **svec, char **dvec);
static void strvec_free(size_t count, char **dvec);

int
ExtSession_init(ExtSession *ls, int translate_crlf, const char *prog, int argc,
	const char **argv, int envc, const char **env_vars,
	const char **env_values)
{
	size_t i, j;
	int master_fd, res;
	char *prog_copy, **argv_copy, *arg, **env_k_copy, **env_v_copy;
	char *slave, *slave_copy;

	prog_copy = strdup(prog);
	err_if(prog_copy == NULL, EXTSESS_MEMORY, res, ProgMallocFailed);

	argv_copy = malloc((argc + 1) * sizeof(char *));
	err_if(argv_copy == NULL, EXTSESS_MEMORY, res, ArgvMallocFailed);
	argv_copy[argc] = NULL;

	res = strvec_copy(argc, argv, argv_copy);
	err_if(res != 0, EXTSESS_MEMORY, res, ArgvArgCopyFailed);

	if (envc > 0) {
		env_k_copy = malloc((envc + 1) * sizeof(char *));
		err_if(env_k_copy == NULL, EXTSESS_MEMORY, res,
			EnvKMallocFailed);
		env_k_copy[argc] = NULL;

		res = strvec_copy(envc, env_vars, env_k_copy);
		err_if(res != 0, EXTSESS_MEMORY, res, EnvKCopyFailed);

		env_v_copy = malloc((envc + 1) * sizeof(char *));
		err_if(env_v_copy == NULL, EXTSESS_MEMORY, res,
			EnvVMallocFailed);
		env_v_copy[argc] = NULL;

		res = strvec_copy(envc, env_values, env_v_copy);
		err_if(res != 0, EXTSESS_MEMORY, res, EnvVCopyFailed);
	} else {
		env_k_copy = NULL;
		env_v_copy = NULL;
	}

	master_fd = posix_openpt(O_RDWR|O_NOCTTY|O_CLOEXEC);
	err_if(master_fd == -1, EXTSESS_OPEN_PTY, res, PtyMasterOpenFailed);

	res = fcntl(master_fd, F_SETFL, O_NONBLOCK);
	err_if(res < 0, EXTSESS_PTY_NONBLOCK, res, PtyNonblockFailed);

	slave = ptsname(master_fd);
	err_if(slave == NULL, EXTSESS_PTS_NAME, res, PtySlaveGetNameFailed);

	slave_copy = strdup(slave);
	err_if(slave_copy == NULL, EXTSESS_MEMORY, res, SlaveMallocFailed);

	ls->prog = prog_copy;
	ls->argc = argc;
	ls->argv = argv_copy;
	ls->envc = envc;
	ls->env_vars = env_k_copy;
	ls->env_values = env_v_copy;
	ls->master_fd = master_fd;
	ls->slave_path = slave_copy;
	ls->childWaitTimer = -1;
	ls->translate_crlf = translate_crlf;

	return EXTSESS_OK;

SlaveMallocFailed:
PtySlaveGetNameFailed:
PtyNonblockFailed:
	close(master_fd);
PtyMasterOpenFailed:
EnvVCopyFailed:
	strvec_free(envc, env_v_copy);
EnvVMallocFailed:
EnvKCopyFailed:
	strvec_free(envc, env_k_copy);
EnvKMallocFailed:
ArgvArgCopyFailed:
	strvec_free(argc, argv_copy);
ArgvMallocFailed:
	free(prog_copy);
ProgMallocFailed:	
	return res;
}

int
ExtSession_run(ExtSession *ls, int stdin_fd, int stdout_fd)
{
	int res;
	alCallback cb;
	pid_t pid;

	res = alEvent_init();
	err_if(res < 0, EXTSESS_AL_EVENT_INIT, res, AlEventInitFailed);

	/* Get callbacks for data available on BBS user data socket */
	AL_CALLBACK(&cb, ls, ExtSession_stdinRead);
	res = alEvent_registerFd(stdin_fd, ALFD_READ, cb, &ls->stdinHandle);
	err_if(res < 0, EXTSESS_REGISTER_STDIN, res, RegisterStdinReaderFailed);

	ls->stdin_fd = stdin_fd;
	ls->stdout_fd = stdout_fd;

	/* Get callbacks from data available from child process */
	AL_CALLBACK(&cb, ls, ExtSession_slaveRead);
	res = alEvent_registerFd(ls->master_fd, ALFD_READ, cb,
		&ls->slaveHandle);
	err_if(res < 0, EXTSESS_REGISTER_SLAVE, res, RegisterSlaveReaderFailed);

	/* Fork */
	pid = fork();
	err_if(pid < 0, EXTSESS_FORK, res, ForkFailed);

	if (pid == 0) {
		/* child */
		ExtSession_execChild(ls);
	}

	ls->childPid = pid;
	ls->childExitCode = -1;

	AL_CALLBACK(&cb, ls, ExtSession_childExit);
	res = alEvent_registerProc(pid, ALPROC_EXIT, cb, &ls->childHandle);
	err_if(res < 0, EXTSESS_REGISTER_CHILD, res, RegisterChildPidFailed);

	while (alEvent_pending())
		alEvent_poll();

	alEvent_shutdown();

	if (ls->childExitCode == -1) {
		res = EXTSESS_UNKNOWN;
	} else if (ls->childExitCode != 0) {
		res = ls->childExitCode;
	} else {
		res = EXTSESS_OK;
	}

	return res;

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

static void
ExtSession_execChild(ExtSession *ls)
{
	size_t i;

	close(ls->master_fd);

	int my_stdout = open(ls->slave_path, O_WRONLY);
	if (my_stdout < 0)
		exit(EXTSESS_CHILD_SLAVE_STDOUT);
	int my_stdin = open(ls->slave_path, O_RDONLY);
	if (my_stdin < 0)
		exit(EXTSESS_CHILD_SLAVE_STDIN);
	int res = dup2(my_stdout, STDOUT_FILENO);
	if (res < 0)
		exit(EXTSESS_CHILD_DUP_STDOUT);
	res = dup2(my_stdin, STDIN_FILENO);
	if (res < 0)
		exit(EXTSESS_CHILD_DUP_STDIN);
	res = dup2(my_stdout, STDERR_FILENO);
	if (res < 0)
		exit(EXTSESS_CHILD_DUP_STDERR);
	close(my_stdout);
	close(my_stdin);
	res = setsid();
	if (res < 0)
		exit(EXTSESS_CHILD_SETSID);
	res = ioctl(STDIN_FILENO, TIOCSCTTY, NULL);
	if (res < 0)
		exit(EXTSESS_CHILD_CTTY);
		
	for (i = 0; i < ls->envc; i++) {
		setenv(ls->env_vars[i], ls->env_values[i], 1);
	}

	execv(ls->prog, ls->argv);

	/* exec failure */
	exit(EXTSESS_CHILD_EXEC);
}

void
ExtSession_deinit(ExtSession *ls)
{
	free(ls->slave_path);
	strvec_free(ls->envc, ls->env_vars);
	strvec_free(ls->envc, ls->env_values);
	strvec_free(ls->argc, ls->argv);
}

/* Stop running due to I/O error or EOF */
static void
ExtSession_stop(ExtSession *ls)
{
	alCallback cb;

	if (ls->stdinHandle != NULL)
		alEvent_deregister(ls->stdinHandle);
	if (ls->slaveHandle != NULL) {
		alEvent_deregister(ls->slaveHandle);
		close(ls->master_fd);
		ls->master_fd = -1;
	}
	ls->slaveHandle = NULL;
	ls->stdinHandle = NULL;
	if (ls->childPid != 0) {
		/* Wait a moment for the child process to exit on its own */
		AL_CALLBACK(&cb, ls, ExtSession_childWaitTimeout);
		ls->childWaitTimer = alEvent_addTimer(5000, 0, cb);
	}
}

static void
ExtSession_stdinRead(void *lsp, void *arg0, int arg1)
{
	ExtSession *ls = lsp;
	ssize_t nread, nwritten;
	char *cr;

	assert(ls->stdinHandle != NULL);

	do {
		nread = read(ls->stdin_fd, ls->buf, sizeof(ls->buf));
		if (nread < 0 && errno != EINTR) {
			fd_printf(ls->stdout_fd, "stdin read: %s\n",
				strerror(errno));
			ExtSession_stop(ls);
			return;
		} else if (nread == 0) {
			ExtSession_stop(ls);
			return;
		}
	} while (nread <= 0);

	if (ls->translate_crlf) {
		while ((cr = memchr(ls->buf, '\r', nread)) != NULL) {
			memmove(cr, cr+1, nread - (cr - ls->buf) - 1);
			nread -= 1;
		}
	} 

	do {
		nwritten = write(ls->master_fd, ls->buf, nread);
		if (nwritten < 0 && errno != EINTR) {
			fd_printf(ls->stdout_fd, "stdin copy: %s\n",
				strerror(errno));
			ExtSession_stop(ls);
			return;
		} else if (nwritten == 0) {
			ExtSession_stop(ls);
			return;
		}
	} while (nwritten <= 0);
}

static void
ExtSession_slaveRead(void *lsp, void *arg0, int arg1)
{
	ExtSession *ls = lsp;
	ssize_t nread;
	char *b, *lf;

	assert(ls->slaveHandle != NULL);

	do {
		nread = read(ls->master_fd, ls->buf, sizeof(ls->buf));
		if (nread < 0 && errno != EINTR) {
			fd_printf(ls->stdout_fd, "slave read: %s\n",
				strerror(errno));
			ExtSession_stop(ls);
			return;
		} else if (nread == 0) {
			ExtSession_stop(ls);
			return;
		}
	} while (nread <= 0);

	if (ls->translate_crlf) {
		b = ls->buf;
		while ((lf = memchr(b, '\n', nread)) != NULL) {
			fd_write(ls->stdout_fd, b, lf - b);
			fd_write(ls->stdout_fd, "\r\n", 2);
			nread -= (lf - b) + 1;
			b = lf + 1;
		}
		if (nread > 0)
			fd_write(ls->stdout_fd, b, nread);
	} else {
		fd_write(ls->stdout_fd, ls->buf, nread);
	}
}

static void
ExtSession_childExit(void *lsp, void *arg0, int arg1)
{
	ExtSession *ls = lsp;
	int res, code, exitcode;

	assert(ls->childHandle != NULL);
	assert(ls->childPid > 0);

	do {
		res = waitpid(ls->childPid, &code, WNOHANG);
		if (res < 0 && errno != EINTR) {
			fd_printf(ls->stdout_fd, "child wait: %s\n",
				strerror(errno));
			ExtSession_childDone(ls, -1);
			return;
		}
	} while (res < 0);

	if (WIFEXITED(code))
		exitcode = WEXITSTATUS(code);
	else
		exitcode = -1;

	ExtSession_childDone(ls, exitcode);
}

static void
ExtSession_childDone(ExtSession *ls, int code)
{
	ls->childPid = 0;
	ls->childExitCode = code;
	alEvent_deregister(ls->childHandle);
	if (ls->childWaitTimer != -1) {
		alEvent_cancelTimer(ls->childWaitTimer);
		ls->childWaitTimer = -1;
	}
	ExtSession_stop(ls);
}

static void
ExtSession_childWaitTimeout(void *lsp, void *arg0, int arg1)
{
	ExtSession *ls = lsp;

	if (ls->childPid != 0) {
		int dummy;
		fd_puts(ls->stdout_fd, "child wait timeout");
		alEvent_deregister(ls->childHandle);
		kill(ls->childPid, SIGKILL);
		waitpid(ls->childPid, &dummy, 0);
		ls->childExitCode = -1;
		ls->childPid = 0;
	}
}

const char *
ExtSession_error(ExtSession_Error err)
{
	switch (err) {
	case EXTSESS_OK: return "None";
	case EXTSESS_MEMORY: return "Out of memory";
	case EXTSESS_OPEN_PTY: return "Open pty";
	case EXTSESS_PTY_NONBLOCK: return "Pty nonblock";
	case EXTSESS_PTS_NAME: return "ptsname";
	case EXTSESS_AL_EVENT_INIT: return "alevent init";
	case EXTSESS_REGISTER_STDIN: return "register stdin";
	case EXTSESS_REGISTER_SLAVE: return "register slave";
	case EXTSESS_FORK: return "fork";
	case EXTSESS_REGISTER_CHILD: return "register child";
	case EXTSESS_CHILD_EXEC: return "exec";
	case EXTSESS_CHILD_SLAVE_STDOUT: return "child stdout";
	case EXTSESS_CHILD_SLAVE_STDIN: return "child stdin";
	case EXTSESS_CHILD_DUP_STDOUT: return "child dup stdout";
	case EXTSESS_CHILD_DUP_STDIN: return "child dup stdin";
	case EXTSESS_CHILD_DUP_STDERR: return "child dup stderr";
	case EXTSESS_CHILD_SETSID: return "child setsid";
	case EXTSESS_CHILD_CTTY: return "child CTTY";
	case EXTSESS_UNKNOWN:
	default:
		return "?";
	}
}

static int
strvec_copy(size_t count, const char **svec, char **dvec)
{
	char *arg;
	size_t i;
	int failed;

	for (i = 0, failed = 0; i < count; i++) {
		if (!failed) {
			arg = strdup(svec[i]);
			if (arg == NULL) {
				failed = -1;
			}
		} else {
			arg = NULL;
		}
		dvec[i] = arg;
	}

	return failed;
}

static void
strvec_free(size_t count, char **vec)
{
	size_t i;

	if (vec == NULL)
		return;

	for (i = 0; i < count; i++) {
		if (vec[i] != NULL) {
			free(vec[i]);
		}
	}

	free(vec);
}
