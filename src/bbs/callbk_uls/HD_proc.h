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
#ifndef HD_PROC_H
#define HD_PROC_H

#include <stdint.h>

#include "field_proc.h"

/*
 * The processing context used for parsing records from a
 * FCC ULS 'HD' (Application/License Header) file.
 */
struct HD_process_ctx_struct {
	uint32_t    HD_uls_id;           /* ULS ID */
	uint16_t    HD_expire_year;      /* License expiration year */
	uint8_t     HD_expire_month;     /* License expiration month */
	uint8_t     HD_expire_day;       /* License expiration day */
};
typedef struct HD_process_ctx_struct HD_process_ctx;

/*
 * The HD processors available.
 */
FPStatus HD_proc_header(void *HD_ctx, char *field_contents);
FPStatus HD_proc_uls_id(void *HD_ctx, char *field_contents);
FPStatus HD_proc_license_status(void *HD_ctx, char *field_contents);
FPStatus HD_proc_expire_date(void *HD_ctx, char *field_contents);

#endif
