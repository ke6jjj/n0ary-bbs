#include <stddef.h>
#include <stdint.h>

#include "call_id.h"

/*
 * Convert a US callsign (up to 2x3) to a unique 32-bit integer
 * identifier. This is an extreme space saving measure.
 *
 * Conceptually, each callsign is first normalized into a 2x3 callsign by
 * inserting spaces. Eg:
 *
 *  A0B    -> _A0__B
 *  A0BC   -> _A0_BC
 *  AB0C   -> AB0__C
 *  AB0CDE -> AB0CDE
 *
 * Then the result is treated as a multi-radix number. That is, each position
 * in the normalized callsign is treated as a digit. Most positions allow for
 * 27 unique digits, the region position allows for only 10. A space counts
 * as a zero digit.
 *
 * The result can be represented as a 6-position vector <a,b,c,d,e,f>.
 * The ID is then:
 *
 * f + e * 27^2 + d * 27^3 + c * 27^4 + b * (27^4 * 10) + a * (27^5 * 10)
 *
 * XXX: This code only works on ASCII machines!
 */
int
call2id(const char *call, uint32_t *r_id)
{
	uint32_t prefix, postfix;
	int step, digit, is_num;
	const char *c;
	size_t end;

	/*
	 * Parse the prefix until the first digit.
	 */
	for (prefix = 0, step = 0, c = call; step < 2; c++, step++) {
		if (*c == '\0')
			break;

		if (*c >= 'a' && *c <= 'z') {
			digit = (*c - 'a') + 1;
		} else if (*c >= 'A' && *c <= 'Z') {
			digit = (*c - 'A') + 1;
		} else if (*c >= '0' && *c <= '9') {
			break;
		} else {
			return -1;
		}

		prefix *= 27;
		prefix += digit;
	}

	if (step == 0)
		/* Unexpected empty prefix */
		return -1;

	if (*c < '0' || *c > '9')
		/* Needed digit here, didn't find one */
		return -1;

	prefix *= 10;
	prefix += (*c - '0');
	c++;

	/*
	 * Parse the postfix.
	 */
	for (step = 0, postfix = 0; step < 3; c++, step++) {
		if (*c == '\0')
			break;

		if (*c >= 'a' && *c <= 'z') {
			digit = (*c - 'a') + 1;
		} else if (*c >= 'A' && *c <= 'Z') {
			digit = (*c - 'A') + 1;
		} else {
			/* No digits or anything else allowed anymore */
			return -1;
		}

		postfix *= 27;
		postfix += digit;
	}

	if (step == 0)
		/* Need at least one suffix character */
		return -1;

	if (*c != '\0')
		/* Too many characters left. */
		return -1;

	*r_id = prefix * (27 * 27 * 27) + postfix;

	return 0;
}

char *
id2call(uint32_t id, char call[7])
{
	int digit, i;
	char *o;

	o = &call[6];
	*o = '\0';

	for (i = 0; i < 3; i++) {
		digit = id % 27;
		if (digit) {
			o--;
			*o = 'A' + digit - 1;
		}
		id /= 27;
	}

	o--;
	*o = (id % 10) + '0';
	id /= 10;

	for (i = 0; i < 2; i++) {
		digit = id % 27;
		if (digit) {
			o--;
			*o = 'A' + digit - 1;
		}
		id /= 27;
	}

	return o;
}

int
id2region(uint32_t id)
{
	/* Remove suffix */
	id /= 27 * 27 * 27;

	return id % 10;
}
