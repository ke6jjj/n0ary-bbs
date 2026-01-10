#include <string.h>

#include "wp.h"

int
safe_get_string(char *buf, char **p, size_t len)
{
	char *res = get_string(p);

	if (res == NULL)
		return -1;

	strlcpy(buf, res, len);

	return 0;
}

int
safe_cat_string(char *buf, char **p, size_t len)
{
	char *res = get_string(p);

	if (res == NULL)
		return -1;

	strlcat(buf, res, len);

	return 0;
}

int
safe_udate2time(time_t *r, char **p)
{
	char *res = get_string(p);

	if (res == NULL)
		return -1;

	return udate2time(res, r);
}

int
safe_get_call(char *buf, char **p, size_t len)
{
	char *res = get_call(p);

	if (res == NULL)
		return -1;

	if (!iscall(res))
		return -1;

	strlcpy(buf, res, len);

	return 0;
}
