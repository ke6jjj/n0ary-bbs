#include <stdio.h>
#include <string.h>

#include "tools.h"

void
safegets(char *buf, size_t sz)
{
	size_t len;
	char *ret;

	ret = fgets(buf, sz, stdin);
	if (ret == NULL) {
		buf[0] = '\0';
		return;
	}

	len = strlen(buf);
	if (len > 0 && buf[len-1] == '\n')
		buf[len-1] = '\0';
}
