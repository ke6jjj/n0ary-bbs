#include <stdio.h>
#include <string.h>
#ifndef SABER
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "c_cmmn.h"
#include "tools.h"

struct text_line *error_list = NULL;

static int in_error_report = FALSE;

int
#ifndef SABER
error_log(char *fmt, ...)
#else
error_log(va_alist) va_dcl
#endif
{
	va_list ap;
	char buf[4096];



#ifndef SABER
	if(in_error_report)
		return ERROR;
	va_start(ap, fmt);
#else
	char *fmt;
	if(in_error_report)
		return ERROR;
	va_start(ap);
	fmt = va_arg(ap, char*);
#endif
	vsprintf(buf, fmt, ap);
	va_end(ap);

	strcat(buf, "\n");
	textline_prepend(&error_list, buf);
	return ERROR;
}

void
error_clear(void)
{
	error_list = textline_free(error_list);
}

void
error_report(void (*callback)(char *s), int clear)
{
	struct text_line *el = error_list;
	in_error_report = TRUE;
	while(el) {
		callback(el->s);
		NEXT(el);
	}

	if(clear)
		error_clear();
	in_error_report = FALSE;
}

static void
error_printf(char *s)
{
	printf("%s", s);
}

void
error_print(void)
{
	error_report(error_printf, TRUE);
}

void
error_print_exit(int code)
{
	error_report(error_printf, TRUE);
	exit(code);
}
