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
#ifndef FIELD_PROC_H
#define FIELD_PROC_H

/*
 * The return codes that a field processing function can use.
 */
enum FPStatus_enum {
	FP_OK,                  /* Field processed ok, move to next one */
	FP_Record_Complete,     /* Record complete, nothing further needed */
	FP_Skip_Record,         /* Skip this record */
	FP_Skip_Record_Warn,    /* Skip this record, show warning */
	FP_Skip_Record_Warn_Ex, /* Skip this record, show warning and field */
};
typedef enum FPStatus_enum FPStatus;

/*
 * Prototype for a record field processing function.
 *
 * The data given to the processor in the "field" argument is guaranteed to
 * stay valid until the record has been processed and the context is reset.
 *
 * Processor may alter characters in the field.
 */
typedef FPStatus (*field_proc_t)(void *ctx, char *field);

#endif
