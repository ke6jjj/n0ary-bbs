#include <syslog.h>
#include <stdarg.h>
#include <stdlib.h>

#include "tools.h"
#include "c_cmmn.h"

static int xlate_bbs_level(int level, int *res);

static int g_bbs_log_level;

void
bbs_log_init(const char *prog, int also_stderr)
{
	int flags = LOG_PID;

	if (also_stderr)
		flags |= LOG_PERROR;

	openlog(prog, flags, BBS_SYSLOG_FACILITY);
	g_bbs_log_level = BBS_LOG_INFO;
	bbs_log_level(g_bbs_log_level);
}

int
bbs_log_level(int level)
{
	int syslog_level, old_level;

	if (xlate_bbs_level(level, &syslog_level) != 0)
		return;

	setlogmask(LOG_UPTO(syslog_level));
	old_level = g_bbs_log_level;
	g_bbs_log_level = level;

	return old_level;
}

void
log_debug(const char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	vsyslog(LOG_DEBUG, message, ap);
	va_end(ap);
}

void
log_info(const char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	vsyslog(LOG_INFO, message, ap);
	va_end(ap);
}

void
log_warning(const char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	vsyslog(LOG_WARNING, message, ap);
	va_end(ap);
}

int
log_error(const char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	vsyslog(LOG_ERR, message, ap);
	va_end(ap);

	return ERROR;
}

void
log_error_exit(int exit_code, const char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	vsyslog(LOG_ERR, message, ap);
	va_end(ap);

	exit(exit_code);
}

static int
xlate_bbs_level(int bbs_level, int *syslog_level)
{
	int level;

	switch (bbs_level) {
	case BBS_LOG_DEBUG:   level = LOG_DEBUG; break;
	case BBS_LOG_INFO:    level = LOG_INFO; break;
	case BBS_LOG_WARNING: level = LOG_WARNING; break;
	case BBS_LOG_ERROR:   level = LOG_ERR; break;
	default:
		return -1;
	}

	*syslog_level = level;

	return 0;
}
