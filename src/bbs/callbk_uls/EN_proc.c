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
#include <string.h>

#include "EN_proc.h"

#include "call_id.h"
#include "common_proc.h"

FPStatus
EN_proc_header(void *_ctx, const char *field_contents)
{
	if (strcmp(field_contents, "EN") != 0)
		return FP_Skip_Record_Warn_Ex;

	return FP_OK;
}

FPStatus
EN_proc_uls_id(void *_ctx, const char *field_contents)
{
	EN_process_ctx *ctx = (EN_process_ctx *)_ctx;

	return common_proc_uls_id(&ctx->EN_uls_id, field_contents);
}

FPStatus
EN_proc_call_sign(void *_ctx, const char *field_contents)
{
	EN_process_ctx *ctx = (EN_process_ctx *)_ctx;
	
	int res = call2id(field_contents, &ctx->EN_call_id);
	if (res != 0)
		return FP_Skip_Record_Warn_Ex;

	return FP_OK;
}

FPStatus 
EN_proc_entity_name(void *_ctx, const char *field_contents)
{
	EN_process_ctx *ctx = (EN_process_ctx *)_ctx;

	ctx->EN_entity_name = field_contents;

	return FP_OK;
}

FPStatus 
EN_proc_first_name(void *_ctx, const char *field_contents)
{
	EN_process_ctx *ctx = (EN_process_ctx *)_ctx;

	ctx->EN_first_name = field_contents;

	return FP_OK;
}

FPStatus 
EN_proc_mi(void *_ctx, const char *field_contents)
{
	EN_process_ctx *ctx = (EN_process_ctx *)_ctx;

	ctx->EN_mi = field_contents;

	return FP_OK;
}

FPStatus 
EN_proc_last_name(void *_ctx, const char *field_contents)
{
	EN_process_ctx *ctx = (EN_process_ctx *)_ctx;

	ctx->EN_last_name = field_contents;

	return FP_OK;
}

FPStatus 
EN_proc_suffix(void *_ctx, const char *field_contents)
{
	EN_process_ctx *ctx = (EN_process_ctx *)_ctx;

	ctx->EN_suffix = field_contents;

	return FP_OK;
}

FPStatus 
EN_proc_street_addr(void *_ctx, const char *field_contents)
{
	EN_process_ctx *ctx = (EN_process_ctx *)_ctx;

	ctx->EN_street_addr = field_contents;

	return FP_OK;
}

FPStatus 
EN_proc_city(void *_ctx, const char *field_contents)
{
	EN_process_ctx *ctx = (EN_process_ctx *)_ctx;

	ctx->EN_city = field_contents;

	return FP_OK;
}

FPStatus 
EN_proc_state(void *_ctx, const char *field_contents)
{
	EN_process_ctx *ctx = (EN_process_ctx *)_ctx;

	ctx->EN_state = field_contents;

	return FP_OK;
}

FPStatus 
EN_proc_zip(void *_ctx, const char *field_contents)
{
	EN_process_ctx *ctx = (EN_process_ctx *)_ctx;

	ctx->EN_zip = field_contents;

	return FP_OK;
}
