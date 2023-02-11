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
#ifndef EM_PROC_H
#define EM_PROC_H

#include <stdint.h>

#include "field_proc.h"

/*
 * The processing context used for parsing records from a
 * FCC ULS 'EN' (Entity) file.
 */
struct EN_process_ctx_struct {
	uint32_t    EN_uls_id;      /* ULS ID */
	uint32_t    EN_call_id;     /* Call Sign (encoded) */
	const char *EN_entity_name; /* Entity Name (for clubs) */
	const char *EN_first_name;  /* First Name */
	const char *EN_mi;          /* MI (Middle Initial) */
	const char *EN_last_name;   /* Last Name */
	const char *EN_suffix;      /* Suffix (for name. E.g. "Jr") */
	const char *EN_street_addr; /* Street Address */
	const char *EN_city;        /* City */
	const char *EN_state;       /* State */
	const char *EN_zip;         /* ZIP code */
};
typedef struct EN_process_ctx_struct EN_process_ctx;

/*
 * The EN field parsers available.
 */
FPStatus EN_proc_header(void *EN_ctx, char *field_contents);
FPStatus EN_proc_uls_id(void *EN_ctx, char *field_contents);
FPStatus EN_proc_call_sign(void *EN_ctx, char *field_contents);
FPStatus EN_proc_entity_name(void *EN_ctx, char *field_contents);
FPStatus EN_proc_first_name(void *EN_ctx, char *field_contents);
FPStatus EN_proc_mi(void *EN_ctx, char *field_contents);
FPStatus EN_proc_last_name(void *EN_ctx, char *field_contents);
FPStatus EN_proc_suffix(void *EN_ctx, char *field_contents);
FPStatus EN_proc_street_addr(void *EN_ctx, char *field_contents);
FPStatus EN_proc_city(void *EN_ctx, char *field_contents);
FPStatus EN_proc_state(void *EN_ctx, char *field_contents);
FPStatus EN_proc_zip(void *EN_ctx, char *field_contents);

#endif
