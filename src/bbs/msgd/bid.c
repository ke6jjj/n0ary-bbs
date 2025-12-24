#include <stdio.h>
#include <string.h>

#include "bid.h"

static int itob36(char *buf, size_t len, unsigned int i);

/*
 * Message numbers 1-99999        : nnnnn_CALL (decimal)
 * Message numbers 100000-60566175: bbbbb.CALL (base36)
 * Message numbers >=60566176     : TODO.
 */
int
format_bid(char *buf, size_t len, unsigned int number, const char *bbs)
{
	size_t actual;

	char b36_buf[6];

	if (number < 100000) {
		actual = snprintf(buf, len, "%u_%s", number, bbs);
		return (actual <= len) ? 0 : -1;
	}

	/* BID needs to use base36 scheme */
	number -= 100000;

	if (itob36(b36_buf, sizeof(b36_buf), number) != 0)
		return -1;

	actual = snprintf(buf, len, "%s.%s", b36_buf, bbs);
	return (actual <= len) ? 0 : -1;
}

static const char b36_alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static int
itob36(char *buf, size_t len, unsigned int i)
{
	size_t p, total_printed;
	unsigned int acc, rem;

	if (len == 0)
		return -1;

	p = len;
	acc = i;

	while (p > 0) {
		rem = acc % 36;
		buf[p-1] = b36_alphabet[rem];
		p = p - 1;
		acc = acc / 36;
		if (acc == 0)
			break;
	}

	if (acc != 0)
		/* Number too big for space provided */
		return -1;

	total_printed = len - p;

	if (p > 0) {
		memmove(&buf[0], &buf[p], total_printed);
	}

	buf[total_printed] = '\0';

	return 0;
}
