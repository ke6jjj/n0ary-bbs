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
#include <stdio.h>
#include <string.h>

#include "HD_proc.h"

#include "common_proc.h"

FPStatus
HD_proc_header(void *_ctx, const char *field_contents)
{
	HD_process_ctx *ctx = (HD_process_ctx *)_ctx;

	if (strcmp(field_contents, "HD") != 0)
		return FP_Skip_Record_Warn_Ex;

	return FP_OK;
}

FPStatus
HD_proc_uls_id(void *_ctx, const char *field_contents)
{
	HD_process_ctx *ctx = (HD_process_ctx *)_ctx;

	return common_proc_uls_id(&ctx->HD_uls_id, field_contents);
}

FPStatus
HD_proc_license_status(void *_ctx, const char *field_contents)
{
	HD_process_ctx *ctx = (HD_process_ctx *)_ctx;

	/* Skip expired licenses. */
	if (strcasecmp(field_contents, "a") != 0)
		/* Not active */
		return FP_Skip_Record;
	
	return FP_OK;
}

FPStatus
HD_proc_expire_date(void *_ctx, const char *field_contents)
{
	HD_process_ctx *ctx = (HD_process_ctx *)_ctx;

	if (*field_contents == '\0')
		/* Empty for whatever reason */
		return FP_Skip_Record;

	int year, month, day;

	int res = sscanf(field_contents, "%d/%d/%d",
		&month,
		&day,
		&year
	);

	if (res != 3)
		/* Parsing problem */
		return FP_Skip_Record_Warn_Ex;

	if (month <= 0 || month > 12 || day <= 0 || day > 31 ||
	    year <= 0)
		/* Weird data */
		return FP_Skip_Record_Warn_Ex;

	ctx->HD_expire_year = (uint16_t) year;
	ctx->HD_expire_month = (uint8_t) month;
	ctx->HD_expire_day = (uint8_t) day;

	return FP_OK;
}
