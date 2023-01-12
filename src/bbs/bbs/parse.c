#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "door.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"
#include "event.h"
#include "tokens.h"
#include "server.h"
#include "user.h"
#include "message.h"
#include "callbk.h"
#include "wp.h"
#include "wx.h"
#include "history.h"
#include "system.h"
#include "parse.h"

#define OPCODE	0
#define OPERAND	1
#define EITHER	2

struct TOKEN *TokenList = NULL;
struct TOKEN *PipeList = NULL, *PipeListEnd = NULL;
char CmdLine[1024];

extern int
	debug_level;

int (*target_func)();

extern int 
	reverse_fwd_mode;

static int
	in_macro = FALSE, macro_num,
	parse_help_command(char *str),
	get_lexem(char **s, struct TOKEN *t, char *upper),
	get_help_token(struct possible_tokens *pt, struct TOKEN *t, char **str),
	get_token(int allowed, struct possible_tokens **PT, struct TOKEN *t, char **str),
	cmd(void);

static void
	free_TokenList(void);

extern struct possible_tokens 
	MaintOpcodes[],
	Opcodes[];

#include "parsetbl.h"

/***************************************************************************
 *	Manipulate the token list, alloc, free, remove
 */

struct TOKEN *
grab_token_struct(struct TOKEN *prev)
{
	struct TOKEN *t = malloc_struct(TOKEN);
	if(TokenList) {
		prev->next = t;
		t->last = prev;
	} else {
		TokenList = t;
		t->last = NULL;
	}
	t->next = NULL;
	return t;
}

static void
free_TokenList(void)
{
	struct TOKEN *t;

	while(TokenList) {
		t = TokenList;
		NEXT(TokenList);
		free(t);
	}
}

void
remove_token(struct TOKEN *t)
{
	if(t == TokenList) {
		TokenList = t->next;
		t->next->last = NULL;
	} else {
		if(t->next)
			t->next->last = t->last;
		if(t->last)
			t->last->next = t->next;
	}
	free(t);
}

void
run_macro_zero(void)
{
	char *s = user_get_macro(0);
	macro_num = 0;
	in_macro = TRUE;
	if(*s != 0)
		parse_command_line(s);
	in_macro = FALSE;
}

static char *
correct_line(char *orig, char *req)
{
	static char buf[1024];
	char *match = &req[1];
	char *replace = (char*)index(match, '^');
	char *p, *q, tmp[1024];

	if(replace == NULL) {
		PRINTF("Bad replacement syntax\n");
		return "";
	}
	*replace++ = 0;

	strcpy(buf, orig);
	if((p = (char*)strstr(buf, match)) == NULL) {
		PRINTF("Pattern not present\n");
		return "";
	}

	q = p;
	q += strlen(match);
	strcpy(tmp, q);

	*p = 0;
	strcat(buf, replace);
	strcat(buf, tmp);

	PRINTF("%s\n", buf);
	return buf;
}

/**************************************************************************
 *	parse a command string
 */

int
parse_command_line(char *cl)
{
	int done = FALSE;
	char *str = cl;
	char new_cmd_line[4096];
	int pipe_pending = FALSE;

		/* Begin by checking for some C-shell features, command
		 * correction and history operations. Both of these will
		 * end up rewriting the command line.
		 */

	if(*str == '^')
		strcpy(new_cmd_line, correct_line(history_last_cmd(), str));
	else {
		strcpy(new_cmd_line, cl);
		if(*str == '!') {
			str++;
			if(*str == '!') {
				str++;
				sprintf(new_cmd_line, "%s %s", history_last_cmd(), str);
			} else
				if(isdigit(*str)) {
					strcpy(new_cmd_line, history_cmd(get_number(&str)));
					sprintf(new_cmd_line, "%s %s", new_cmd_line, str);
				}
		}
	}

		/* add new command line to history list and advance our pointer
		 * to the first non-blank character.
		 */
	history_add(new_cmd_line);
	str = new_cmd_line;
	do {
		char *begin;
		struct possible_tokens *pt = Opcodes;
		struct TOKEN *t;
		int allowed = pUSER;

		no_prompt = FALSE;

			/* determine what command set we are allowed to use */

		if(ImSysop)
			allowed = pSYSOP;
		else
			switch(batch_mode) {
			case FALSE:
				allowed = pUSER;
				break;
			case TRUE:
				allowed = pBATCH;
				break;
			case ERROR:
				allowed = pRESTRICT;
				break;
			}

			/* skip past any leading BREAKs, ';' */

		NextChar(str);
		while(*str == ';') {
			str++;
			NextChar(str);
		}

		begin = str;
		strcpy(CmdLine, str);

		if(debug_level & DBG_TOKENS)
			PRINTF("CmdLine: %s\n", CmdLine);

			/* if a macro is requested, starts with a number, then
			 * handle it now.
			 */

		if(isdigit(*str)) {
			char *p = str;
			if(debug_level & DBG_TOKENS)
				PRINTF("loc1: %s\n", str);
			p++;
			NextChar(p);

					/* it makes a difference what the next character is.
					 * if this is the last token then we just parse this
					 * command line and return.
					 */
			if(debug_level & DBG_TOKENS)
				PRINTF("loc2: %s\n", str);
			if(!*p) {
				char *macro_str;
				macro_num = atoi(str);
				in_macro = TRUE;

			if(debug_level & DBG_TOKENS)
				PRINTF("loc3: %s\n", str);
				macro_str = user_get_macro(macro_num);
				if(*macro_str != 0)
					parse_command_line(macro_str);

				in_macro = FALSE;
				return OK;
			}

					/* if the next character is a BREAK character then we
					 * have more parsing to do so save our state.
					 */

			if(*p == ';') {
				char *macro_str;
				char *save = NULL;
				int save_in_macro = in_macro;
				int save_macro_num = macro_num;

				save = copy_string(&p[1]);
				if(debug_level & DBG_TOKENS)
					PRINTF("parse_macro_num: %s\n", str);
				macro_num = atoi(str);
				in_macro = TRUE;

				macro_str = user_get_macro(macro_num);
				if(debug_level & DBG_TOKENS)
					PRINTF("Macro %d: %s\n", macro_num, macro_str);
				if(*macro_str != 0)
					parse_command_line(macro_str);

				in_macro = save_in_macro;
				macro_num = save_macro_num;
				str = save;
				free(save);
				save = NULL;
				NextChar(str);

				if(debug_level & DBG_TOKENS)
					PRINTF("Recover: %s\n", str);
				continue;
			}
		}

		if(TokenList)
			free_TokenList();

		if(in_macro) {
			char *p, cmdbuf[80];
			strcpy(cmdbuf, str);
			if((p = (char*)index(cmdbuf, ';')) != NULL)
				*p = 0;
			PRINTF("\n{macro %d: %s}\n", macro_num, cmdbuf);
		}

		target_func = NULL;
		t = grab_token_struct(NULL);

		NextChar(str);
		while(*str == ';') {
			str++;
			NextChar(str);
		}
		if(*str == 0) {
			done = TRUE;
			break;
		}

		t->location = str - begin;
		get_token(allowed, &pt, t, &str);

		if(t->token == HELP) {
			parse_help_command(str);
			return help();
		}

		if(t->token == END) {
			done = TRUE;
			break;
		}

		if(t->token == UNKNOWN) {
			PRINT(CmdLine);
			PRINTF("\n^-- Invalid command \"%s\"\n", TokenList->lexem);
			free_TokenList();
			if(ImBBS)
				exit_bbs();
			return ERROR;
		}

		if(t->auxtoken) {
			t = grab_token_struct(t);
			t->token = t->last->auxtoken;
		}

		do {
			t = grab_token_struct(t);
			t->location = str - begin;
			get_token(allowed, &pt, t, &str);
				
			t->target_func = target_func;

			if(t->auxtoken) {
				t = grab_token_struct(t);
				t->token = t->last->auxtoken;
			}

		} while(t->token != END && t->token != BREAK);

		if(t->token == END)
			done = TRUE;

		if(pipe_pending) {
			if(PipeList) {
				t->last->next = PipeList;
				PipeList->last = t->last;

				PipeListEnd->next = t;
				t->last = PipeListEnd;
				PipeList = PipeListEnd = NULL;

				if(debug_level & DBG_TOKENS)
					display_tokens();
				bbsd_msg(CmdLine);
				target_func();
				pipe_pending = FALSE;
			}

		} else {
			if(debug_level & DBG_TOKENS)
				display_tokens();

			if(t->last->token == PIPE)
				pipe_pending = TRUE;
			t->token = END;
			bbsd_msg(CmdLine);
			target_func();
		}

	} while(!done);

	return OK;
}

static int
get_lexem(char **s, struct TOKEN *t, char *upper)
{
	int cnt = 0;
	int alpha = 0;
	int number = 0;
	int other = 0;
	char *actual = t->lexem;

	if(!**s) {
		t->token = END;
		return 0;
	}
	t->token = 0;

#if 0
	/* this used to be for splitting the callarea from the suffix
	 * but it caused problems with the reconstruction of BIDS.
	 * keep this in mind if you are thinking about how neat it would
	 * be to strip a number.
	 */
	if(isdigit(**s))
		while(isdigit(**s)) {
			*actual++ = *upper = **s;
			number++;
			(*s)++;
			cnt++;
		}
	else
#endif
				/* I added the '/' to this because someone felt that
				 * the category "NOV/TE" would be a good idea, stupid.
				 * but it took the bbs down for 3 days.
				 *
				 * Also allow date and time to be issued as a single
				 * token, '/', ':'.
				 */

	while(isalnum(**s) || **s == '!' || **s == '_' || **s == '/' || **s == ':') {
		*actual++ = *upper = **s;
		if(islower(*upper))
			*upper = toupper(*upper);
		upper++;
	
		if(isalpha(**s))
			alpha++;
		else if(isdigit(**s))
			number++;
		else
			other++;

		*upper = *actual = 0;

		(*s)++;
		cnt++;
	}

	if(!cnt) {
		if(**s == '"') {
			(*s)++;
			while(**s && **s != '"')
				*actual++ = *upper++ = *(*s)++;
			*upper = *actual = 0;
			if(**s == '"')
				(*s)++;
			t->token = STRING;
			while(isspace(**s))
				(*s)++;
			return 0;
		}
				
		*actual = *upper = *(*s)++;
		cnt++;
	}

	while(isspace(**s))
		(*s)++;

	if(other)
		t->token = STRING;
	else {
		if(number && !alpha) {
			t->token = NUMBER;
			t->value = atoi(t->lexem);
			return 0;
		} else
			if(alpha)
				t->token = WORD;
			else
				t->token = UNKNOWN;
	}

	return cnt;
}

static int
get_token(int allowed, struct possible_tokens **PT, struct TOKEN *t, char **str)
{
	char lexem[1024];
	struct possible_tokens *new_pt = *PT;
	int len = get_lexem(str, t, lexem);

	if(target_func == NULL && !InServer)
		t->token = UNKNOWN;

	if(len)
		while((*PT)->str) {
			struct possible_tokens *pt = (*PT)++;

			if((pt->allowed & allowed) == 0)
				continue;

			if(pt->max < len)
				continue;
			if(pt->min > len)
				continue;

			if(strncmp(lexem, pt->str, len))
				continue;

					/* match found */

			t->token = pt->token;
			t->auxtoken = pt->auxtoken;
			if(pt->op_tbl)
				new_pt = pt->op_tbl;
			if(pt->func)
				target_func = pt->func;
			break;
		}	

	*PT = new_pt;
	return OK;
}

void
pipe_msg_num(int num)
{
	struct TOKEN *t = malloc_struct(TOKEN);

	if(PipeList) {
		PipeListEnd->next = t;
		t->last = PipeListEnd;
		NEXT(PipeListEnd);
	} else {
		PipeList = PipeListEnd = t;
		t->last = NULL;
	}
	t->next = NULL;
	t->location = 0;
	t->token = NUMBER;
	t->value = num;
	sprintf(t->lexem, "%d", num);
}

/****************************************************************************
 *	parsing a help request is slightly different than a real command
 */

static int
parse_help_command(char *str)
{
	struct TOKEN *t = NULL;

	if(TokenList)
		free_TokenList();

	do {
		t = grab_token_struct(t);
		get_help_token(HelpOperands, t, &str);

	} while(t->token != END);
	return OK;
}

static int
get_help_token(struct possible_tokens *pt, struct TOKEN *t, char **str)
{
	char lexem[1024];
	int len = get_lexem(str, t, lexem);

	if(t->token == NUMBER)
		return OK;

	if(len)
		while(pt->str) {

			if(pt->max == 0)
				if(pt->str[0] == lexem[0]) {
					t->token = pt->token;
					t->value = pt->auxtoken;
					break;
				}

			if(pt->max >= len && pt->min <= len)
				if(!strncmp(lexem, pt->str, len)) {
					t->token = pt->token;
					t->value = pt->auxtoken;
					break;
				}	

			pt++;
		}

	return OK;
}

/****************************************************************************
 *	parse a string other than a command line.
 */

int
parse_simple_string(char *str, struct possible_tokens *pt)
{
	struct TOKEN *t = NULL;
	int allowed = pUSER|pSYSOP|pBATCH|pRESTRICT;

	if(TokenList)
		free_TokenList();

	do {
		t = grab_token_struct(t);
		get_token(allowed, &pt, t, &str);
				
	} while(t->token != END);
	return OK;
}

/****************************************************************************
 */

static int
cmd(void)
{
	struct TOKEN *t = TokenList;

	switch(t->token) {
	case ECHOCMD:
		NEXT(t);
		while(t->token != END) {
			PRINTF("%s ", t->lexem);
			NEXT(t);
		}
		PRINTF("\n");
		break;
	case MACRO:
		parse_command_line(user_get_macro(atoi(t->lexem)));	
		break;
	}
	return OK;
}

struct TOKEN *
resync_token_via_location(struct TOKEN *t, char *new_loc)
{
	char *p = new_loc;

	while(t->token != END) {
		if(p == &CmdLine[t->location])
			return t;
		if(p < &CmdLine[t->location]) {
			PRINT("Possible token problem, please send the command you just\n");
			PRINT("to the SYSOP. Attempting to continue ...\n");
			return t;
		}
		NEXT(t);
	}
	return t;
}

