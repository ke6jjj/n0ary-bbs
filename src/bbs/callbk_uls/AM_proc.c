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
#include <stdlib.h>

#include "AM_proc.h"

#include "common_proc.h"

FPStatus
AM_proc_header(void *_ctx, char *field_contents)
{
	AM_process_ctx *ctx = (AM_process_ctx *)_ctx;

	if (strcmp(field_contents, "AM") != 0)
		return FP_Skip_Record_Warn_Ex;

	return FP_OK;
}

FPStatus
AM_proc_uls_id(void *_ctx, char *field_contents)
{
	AM_process_ctx *ctx = (AM_process_ctx *)_ctx;
 
	return common_proc_uls_id(&ctx->AM_uls_id, field_contents);
}

FPStatus
AM_proc_op_class(void *_ctx, char *field_contents)
{
	AM_process_ctx *ctx = (AM_process_ctx *)_ctx;

	if (field_contents[0] == '\0') {
		/* Empty class usually means a club call */
		ctx->AM_op_class = 'C';
	} else {
		ctx->AM_op_class = field_contents[0];
	}

	return FP_OK;
}
