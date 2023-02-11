/*
 * Copyright 2022, Jeremy Cooper.  All rights reserved.
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
#include <string.h>

#include "CA_proc.h"

#include "call_id.h"
#include "common_proc.h"

#define STRAIGHT_COPY(func, destination_field)        \
FPStatus                                              \
func(void *_ctx, char *field_contents)                \
{                                                     \
	CA_process_ctx *ctx = (CA_process_ctx *)_ctx; \
	ctx->destination_field = field_contents;      \
	return FP_OK;                                 \
}

#define NONEMPTY_UINT8(func, destination_field)          \
FPStatus                                                 \
func(void *_ctx, char *field_contents)                   \
{                                                        \
	CA_process_ctx *ctx = (CA_process_ctx *)_ctx;    \
	ctx->destination_field = field_contents[0] != 0; \
	return FP_OK;                                    \
}

FPStatus
CA_proc_call_sign(void *_ctx, char *field_contents)
{
	CA_process_ctx *ctx = (CA_process_ctx *)_ctx;
	
	int res = call2id(field_contents, &ctx->CA_call_id);
	if (res != 0)
		return FP_Skip_Record_Warn_Ex;

	return FP_OK;
}

FPStatus
CA_proc_first_name(void *_ctx, char *field_contents)
{
	CA_process_ctx *ctx = (CA_process_ctx *)_ctx;
	char *first_name, *second_name;
	
	/* Detect second names */
	ctx->CA_first_name = second_name = field_contents;
	strsep(&second_name, " ");

	if (second_name != NULL) {
		ctx->CA_middle_name = second_name;
	} else {
		ctx->CA_middle_name = "";
	}

	return FP_OK;
}

STRAIGHT_COPY(CA_proc_surname, CA_surname)
STRAIGHT_COPY(CA_proc_address, CA_addr_line)
STRAIGHT_COPY(CA_proc_city, CA_city)
STRAIGHT_COPY(CA_proc_province, CA_province)
STRAIGHT_COPY(CA_proc_postal_code, CA_postal_code)
NONEMPTY_UINT8(CA_proc_qual_a, CA_qual_a)
NONEMPTY_UINT8(CA_proc_qual_b, CA_qual_b)
NONEMPTY_UINT8(CA_proc_qual_c, CA_qual_c)
NONEMPTY_UINT8(CA_proc_qual_d, CA_qual_d)
NONEMPTY_UINT8(CA_proc_qual_e, CA_qual_e)
STRAIGHT_COPY(CA_proc_club_name, CA_club_name)
STRAIGHT_COPY(CA_proc_club_name2, CA_club_name2)
STRAIGHT_COPY(CA_proc_club_addr, CA_club_addr)
STRAIGHT_COPY(CA_proc_club_city, CA_club_city)
STRAIGHT_COPY(CA_proc_club_prov, CA_club_prov)
STRAIGHT_COPY(CA_proc_club_postal, CA_club_postal)
