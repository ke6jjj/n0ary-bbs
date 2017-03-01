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
#ifndef MAKECB_ULS_CALL_ID_H
#define MAKECB_ULS_CALL_ID_H

#include <stdint.h>

int call2id(const char *call, uint32_t *r_id);
char *id2call(uint32_t id, char call[7]);
int   id2region(uint32_t id);

#endif
