/*
 * Copyright 2017, Jeremy Cooper.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Jeremy Cooper.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "cb_date.h"

/*
 * N0ARY's callbook format stores dates as year (without century) and day of
 * year.
 *
 * year - Canonical year, CE. (1990 CE = 1990)
 * month - Canonical month number (1 = January)
 * day - Canonical day number (1 = first day of the month)
 */
uint16_t
get_doy(unsigned int year, uint8_t month, uint8_t day)
{
	/*
	 * Given a month in the year (0 = January), return the day
	 * of the year in which the month's first day starts, under a non-
	 * leap year.
	 */
	static const int days[12] = {
		/* Jan */ 0,
		/* Feb */ 31,
		/* Mar */ 31+28,
		/* Apr */ 31+28+31,
		/* May */ 31+28+31+30,
		/* Jun */ 31+28+31+30+31,
		/* Jul */ 31+28+31+30+31+30,
		/* Aug */ 31+28+31+30+31+30+31,
		/* Sep */ 31+28+31+30+31+30+31+31,
		/* Oct */ 31+28+31+30+31+30+31+31+30,
		/* Nov */ 31+28+31+30+31+30+31+31+30+31,
		/* Dec */ 31+28+31+30+31+30+31+31+30+31+30
	};
	int is_leap, doy;

	if ((year % 400) == 0)
		is_leap = 1;
	else if ((year % 100) == 0)
		is_leap = 0;
	else if ((year % 4) == 0)
		is_leap = 1;
	else
		is_leap = 0;

	doy = days[month-1] + day - 1;

	if (is_leap && month >= 3)
		/* Account for February having 29 days in a leap year */
		/* instead of the encoded number, which is 28.        */
		doy += 1;

	return doy;
}
