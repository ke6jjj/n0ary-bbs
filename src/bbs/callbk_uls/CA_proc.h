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
#ifndef CA_PROC_H
#define CA_PROC_H

#include <stdint.h>

#include "field_proc.h"

/*
 * The processing context used for parsing records from an amateur callsign
 * list from Industry Canada.
 */
struct CA_process_ctx_struct {
	uint32_t    CA_call_id;     /* Call Sign (encoded) */
	const char *CA_first_name;  /* First Name */
	const char *CA_middle_name; /* Middle Name (synthesized) */
	const char *CA_surname;     /* Surname */
	const char *CA_addr_line;   /* Street Address */
	const char *CA_city;        /* City */
	const char *CA_province;    /* Province */
	const char *CA_postal_code; /* Postal Code */
	const char *CA_club_name;   /* Club Name */
	const char *CA_club_name2;  /* Club Name2 */
	const char *CA_club_addr;   /* Club Address */
	const char *CA_club_city;   /* Club City */
	const char *CA_club_prov;   /* Club Province */
	const char *CA_club_postal; /* Club Postal Code */
	uint8_t     CA_qual_a;      /* Qualification A */
	uint8_t     CA_qual_b;      /* Qualification B */
	uint8_t     CA_qual_c;      /* Qualification C */
	uint8_t     CA_qual_d;      /* Qualification D */
	uint8_t     CA_qual_e;      /* Qualification E */
};
typedef struct CA_process_ctx_struct CA_process_ctx;

/*
 * The CA field parsers available.
 */
FPStatus CA_proc_header(void *EN_ctx, char *field_contents);
FPStatus CA_proc_call_sign(void *EN_ctx, char *field_contents);
FPStatus CA_proc_first_name(void *EN_ctx, char *field_contents);
FPStatus CA_proc_surname(void *EN_ctx, char *field_contents);
FPStatus CA_proc_address(void *EN_ctx, char *field_contents);
FPStatus CA_proc_city(void *EN_ctx, char *field_contents);
FPStatus CA_proc_province(void *EN_ctx, char *field_contents);
FPStatus CA_proc_postal_code(void *EN_ctx, char *field_contents);
FPStatus CA_proc_qual_a(void *EN_ctx, char *field_contents);
FPStatus CA_proc_qual_b(void *EN_ctx, char *field_contents);
FPStatus CA_proc_qual_c(void *EN_ctx, char *field_contents);
FPStatus CA_proc_qual_d(void *EN_ctx, char *field_contents);
FPStatus CA_proc_qual_e(void *EN_ctx, char *field_contents);
FPStatus CA_proc_club_name(void *EN_ctx, char *field_contents);
FPStatus CA_proc_club_name2(void *EN_ctx, char *field_contents);
FPStatus CA_proc_club_addr(void *EN_ctx, char *field_contents);
FPStatus CA_proc_club_city(void *EN_ctx, char *field_contents);
FPStatus CA_proc_club_prov(void *EN_ctx, char *field_contents);
FPStatus CA_proc_club_postal(void *EN_ctx, char *field_contents);

#endif
