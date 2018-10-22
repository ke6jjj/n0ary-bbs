#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "tokens.h"
#include "message.h"
#include "user.h"
#include "wp.h"
#include "vars.h"
#include "function.h"
#include "system.h"
#include "filesys.h"

extern int
	debug_level;

int
user_disp_field(int token)
{
	int i;

	switch(token) {
	case WHO:
		user_disp_account(usercall);
		break;
	case HOME:
		PRINTF("%s\n",  wp_get_field(usercall, wHOME)); break;
	case FNAME:
		PRINTF("%s\n",  wp_get_field(usercall, wFNAME)); break;
	case QTH:
		PRINTF("%s\n",  wp_get_field(usercall, wQTH)); break;
	case ZIP:
		PRINTF("%s\n",  wp_get_field(usercall, wZIP)); break;

	case COMPUTER:
		PRINTF("%s\n",  user_get_field(uCOMPUTER)); break;
	case EMAIL:
		PRINTF("EMail\n");
		if(user_check_flag(uEMAIL))
			PRINTF("   forwarding: ON for %s mail\n",
				user_check_flag(uEMAILALL) ? "ALL":"LOCAL");
		else
			PRINTF("   forwarding: OFF\n");
		PRINTF("      address: %s\n", user_get_field(uADDRESS));
		if(gated_open() == OK) {
			char cmd[80];
			PRINTF("\nIncoming allowed from:\n");
			sprintf(cmd, "USER %s", usercall);
			if(gated_fetch_multi(cmd, usr_disp_gateallow) == ERROR)
				PRINTF("\tNONE\n");
			gated_close();
		}
		break;
	case EXCLUDE:
		user_get_list(uEXCLUDE);
		usr_disp_list(Exclude);
		break;

	case INCLUDE:
		user_get_list(uINCLUDE);
		usr_disp_list(Include);
		break;

	case FREQ:
		PRINTF("%s\n",  user_get_field(uFREQ)); break;
	case DUPLEX:
		PRINTF("%s\n",  user_check_flag(uHDUPLEX) ? "HALF":"FULL"); break;

	case HELP:
		PRINTF("Help level: %d\n",  user_get_value(uHELP)); break;
	case LINES:
		PRINTF("%d\n",  user_get_value(uLINES)); break;
	case BASE:
		PRINTF("%d\n",  user_get_value(uBASE)); break;

	case LNAME:
		PRINTF("%s\n",  user_get_field(uLNAME)); break;
	case MACRO:
		for(i=0; i<10; i++)
			PRINTF("MACRO %d: %s\n", i, user_get_macro(i));
		break;
	case NEWLINE:
		PRINTF("%s\n",  user_check_flag(uNEWLINE) ? "ON":"OFF"); break;
	case UPPERCASE:
		PRINTF("%s\n",  user_check_flag(uUPPERCASE) ? "ON":"OFF"); break;
	case PHONE:
		PRINTF("%s\n",  user_get_field(uPHONE)); break;
	case REGEXP:
		PRINTF("Regular Expression search is %s\n",
			user_check_flag(uREGEXP) ? "ON":"OFF");
		break;
	case RIG:
		PRINTF("%s\n",  user_get_field(uRIG)); break;
	case SIGNATURE:
		PRINTF("Automatic signature is %s\n",
			user_check_flag(uSIGNATURE) ? "ON":"OFF");
		break;
	case SOFTWARE:
		PRINTF("%s\n",  user_get_field(uSOFTWARE)); break;
	case TNC:
		PRINTF("%s\n",  user_get_field(uTNC)); break;
	case VACATION:
		PRINTF("Vacation mode is %s\n",
			user_check_flag(uVACATION) ? "ON":"OFF"); break;
	case EQUIP:
		PRINTF("Computer: %s\n",  user_get_field(uCOMPUTER));
		PRINTF("Software: %s\n",  user_get_field(uSOFTWARE));
		PRINTF("     Rig: %s\n",  user_get_field(uRIG));
		PRINTF("     TNC: %s\n",  user_get_field(uTNC));
		break;
	default:
		PRINTF("usr_disp_field(%d) ... internal error [token not found]\n",
			token);
		exit(1);
	}
	return OK;
}

static void
u_flag(int token, char *toggle)
{
	switch(token) {
	case NEWLINE:
		user_flag(uNEWLINE, toggle); break;
	case REGEXP:
		user_flag(uREGEXP, toggle); break;
	case SIGNATURE:
		user_flag(uSIGNATURE, toggle); break;
	case VACATION:
		user_flag(uVACATION, toggle); break;
	case UPPERCASE:
		user_flag(uUPPERCASE, toggle); break;
	}
}
	
static void
u_add_list(int token, struct TOKEN *t)
{
	char buf[256];
	int key = '>';

	buf[0] = 0;
	while(t->token != END) {
		switch(t->token) {
		case TO:	key = '>'; break;
		case FROM:	key = '<'; break;
		case AT:	key = '@'; break;

		case WORD:
			sprintf(buf, "%s %c%s", buf, key, t->lexem);
			key = '>';
			break;

		default:
			bad_cmd(-1, t->location);
			return;
		}
		NEXT(t);
	}
	if(buf[0])
		user_set_field(token, buf);
}

static void
u_del_list(int token)
{
	struct IncludeList *last = NULL, *list, **List;
	int changed = FALSE;

	user_get_list(token);
	List = (token == uEXCLUDE) ? &Exclude : &Include;

	list = *List;
	while(list) {
		struct IncludeList *t = list;
		PRINTF("%s  Delete (N/y/q)? ", usr_disp_list_entry(list));
		switch(get_yes_no_quit(NO)) {
		case YES:
			t = list;
			if(last == NULL)
				*List = list->next;
			else
				last->next = list->next;
			NEXT(list);

			while(t) {
				struct IncludeList *tp = t;
				t = t->comp;
				free(tp);
			}
			changed = TRUE;
			continue;

		case NO:
			break;
		case QUIT:
		case ERROR:
			return;
		}
		last = list;
		NEXT(list);
	}

	if(changed) {
		char buf[256];
		user_set_field(token, "");
		list = *List;
		while(list) {
			struct IncludeList *t = list;
			buf[0] = 0;
			while(t) {
				sprintf(buf, "%s %c%s", buf, t->key, t->str);			
				t = t->comp;
			}
			NEXT(list);
			if(buf[0])
				user_set_field(token, buf);
		}
	}
}

static void
u_list(int token, struct TOKEN *t)
{
	flush_message_list();

	switch(t->token) {
	case END:
		break;
	case DELETE:
		u_del_list(token);
		break;
	case KILL:
		user_set_field(token, "");
		break;
	case ADD:
		NEXT(t);
	default:
		u_add_list(token, t);
		break;
	}
}

static void
user_disp_brief(char *s)
{
	char call[80], port[80];
	int cnt;
	time_t t;

	strcpy(call, get_string(&s));
	cnt = get_number(&s);
	strcpy(port, get_string(&s));
	t = (time_t)get_number(&s);

	PRINTF("%s\t%d\t%s\t   %s", call, cnt, port_alias(port), ctime(&t));
}

void
user_disp_last_connects(struct TOKEN *t)
{
	char cmd[80];
	int cnt = 10;

	PRINTF(" Call   Cnt       Port\n");

	switch(t->token) {
	case NUMBER:
		cnt = t->value;
		break;

	case WORD:
		while(t->token != END) {
			char *result;

			if(t->token == WORD) {
				sprintf(cmd, "USER %s", t->lexem);
				result = userd_fetch(cmd);
				if(!strncmp(result, "NO,", 3)) {
					uppercase(t->lexem);
					PRINTF("%s\t\t\t   Not a current user of this bbs\n",
						t->lexem);
				} else
					user_disp_brief(result);
			}
			NEXT(t);
		}
		return;
	}

	sprintf(cmd, "USER %d", cnt);
	userd_fetch_multi(cmd, user_disp_brief);
	return;
}

int
usr(void)
{
	struct TOKEN *t = TokenList;
	char *field = NULL;
	char *result;
	char *p;
	int token = t->token;

	switch(token) {
	case BBSSID:
		/* It would be nice to have access to the Systems file at this point
		 * so that we could compare the SIDs. Unfortunantely it is still 
		 * a bit early.
		 */
		if(parse_bbssid(&CmdLine[t->location], 
						system_get_exp_sid(usercall))== ERROR) {
			PRINTF("*** your sid doesn't match expected\n");
			exit_bbs();
		}
		return OK;
	case ME:
		user_disp_account(usercall);
		return OK;
	case USER:
		user_disp_last_connects(t->next);
		return OK;
	}

	NEXT(t);
	if(t->token == END)
		return user_disp_field(token);

	switch(token) {

	case WHO:
		user_disp_account(t->lexem);
		break;

		/* WP Variables */

	case HOME:
		field = copy_string(t->lexem);
		if(strlen(field) > LenCALL)
			field[LenCALL-1] = 0;
		wp_set_users_homebbs(usercall, field, wUSER);
		free(field);
		break;
	case FNAME:
		p = &CmdLine[t->location];
		field = copy_string(get_string(&p));
		if(strlen(field) > LenFNAME)
			field[LenFNAME-1] = 0;
		wp_set_field(usercall, wFNAME, wUSER, field);
		free(field);
		break;
	case QTH:
		field = copy_string(&CmdLine[t->location]);
		if(strlen(field) > LenQTH)
			field[LenQTH-1] = 0;
		wp_set_field(usercall, wQTH, wUSER, field);
		free(field);
		break;
	case ZIP:
		field = copy_string(t->lexem);
		if(strlen(field) > LenZIP)
			field[LenZIP-1] = 0;
		wp_set_field(usercall, wZIP, wUSER, field);
		free(field);
		break;

		/* Raw fields, not checking */

	case LNAME:
		field = copy_string(&CmdLine[t->location]);
		if(strlen(field) > LenLNAME)
			field[LenLNAME-1] = 0;
		user_set_field(uLNAME, field);
		free(field);
		break;
	case COMPUTER:
		field = copy_string(&CmdLine[t->location]);
		if(strlen(field) > LenEQUIP)
			field[LenEQUIP-1] = 0;
		user_set_field(uCOMPUTER, field);
		free(field);
		break;
	case FREQ:
		field = copy_string(&CmdLine[t->location]);
		if(strlen(field) > LenFREQ)
			field[LenFREQ-1] = 0;
		user_set_field(uFREQ, field);
		free(field);
		break;
	case PHONE:
		field = copy_string(&CmdLine[t->location]);
		if(strlen(field) > LenPHONE)
			field[LenPHONE-1] = 0;
		user_set_field(uPHONE, field);
		free(field);
		break;
	case RIG:
		field = copy_string(&CmdLine[t->location]);
		if(strlen(field) > LenEQUIP)
			field[LenEQUIP-1] = 0;
		user_set_field(uRIG, field);
		free(field);
		break;
	case SOFTWARE:
		field = copy_string(&CmdLine[t->location]);
		if(strlen(field) > LenEQUIP)
			field[LenEQUIP-1] = 0;
		user_set_field(uSOFTWARE, field);
		free(field);
		break;
	case TNC:
		field = copy_string(&CmdLine[t->location]);
		if(strlen(field) > LenEQUIP)
			field[LenEQUIP-1] = 0;
		user_set_field(uTNC, field);
		free(field);
		break;

		/* Toggles, expect ON/OFF */

	case UPPERCASE:
	case NEWLINE:
	case REGEXP:
		switch(t->token) {
		case ON:
			u_flag(token, "ON");
			break;
		case OFF:
			u_flag(token, "OFF");
			break;
		default:
			PRINTF("Expected ON or OFF\n");
		}
		break;
	case SIGNATURE:
		switch(t->token) {
		case ON:
			if(signature_file_exists())
				u_flag(token, "ON");
			else
				PRINTF("SIGNATURE cannot be enabled without a SIGNATURE file\n");
			break;
		case OFF:
			u_flag(token, "OFF");
			break;
		default:
			PRINTF("Expected ON or OFF\n");
		}
		break;

	case VACATION:
		switch(t->token) {
		case ON:
			if(vacation_file_exists())
				u_flag(token, "ON");
			else
				PRINTF("VACATION cannot be enabled without a VACATION file\n");
			break;
		case OFF:
			u_flag(token, "OFF");
			break;
		default:
			PRINTF("Expected ON or OFF\n");
		}
		break;

		/* Expect NUMBER */

	case HELP:
		if(t->token == NUMBER)
			if(t->value >= 0 && t->value <= 3) {
				user_set_value(uHELP, t->value);
				MyHelpLevel = t->value;
				break;
			}
		PRINTF("Expect help levels, 0-3\n");
		break;
		
	case LINES:
		if(t->token == NUMBER)
			user_set_value(uLINES, t->value);
		else
			PRINTF("Expected a number\n");
		break;

	case BASE:
		if(t->token == NUMBER)
			user_set_value(uBASE, t->value);
		else
			PRINTF("Expected a number\n");
		break;

		/* Complex, ON/OFF or string */

	case EMAIL:
		switch(t->token) {
		case ON:
		case ALL:
			result = user_get_field(uADDRESS);
			if(*result == 0) {
				PRINTF("An EMAIL address must be set before enabling function\n");
				break;
			}
			user_set_flag(uEMAIL);
			user_set_flag(uEMAILALL);
			break;
		case LOCAL:
			result = user_get_field(uADDRESS);
			if(*result == 0) {
				PRINTF("An EMAIL address must be set before enabling function\n");
				break;
			}
			user_set_flag(uEMAIL);
			user_clr_flag(uEMAILALL);
			break;
		case OFF:
			user_clr_flag(uEMAIL);
			user_clr_flag(uEMAILALL);
			break;
		default:
			{
				char *address = copy_string(&CmdLine[t->location]);
				char *domain;

				address[LenEMAIL-1] = 0;
				domain = (char *)rindex(address, '@');
				
				if(domain != NULL) {
					domain++;
					if(!stricmp(domain, Bbs_Domain)) {
						PRINTF("EMAIL loop detected\n");
						break;
					}
				}

				user_set_field(uADDRESS, address);
				free(address);
			}
			break;
		}
		break;

	case EXCLUDE:
		u_list(uEXCLUDE, t);
		break;
	case INCLUDE:
		u_list(uINCLUDE, t);
		break;

	case DUPLEX:
		PRINTF("%s\n",  user_check_flag(uHDUPLEX) ? "HALF":"FULL"); break;

	case MACRO:
		if(t->token == NUMBER)
			if(t->value >= 0 && t->value <= 9) {
				int number = t->value;
				NEXT(t);
				if(t->token == END)
					user_set_macro(number, "");
				else {
					field = copy_string(&CmdLine[t->location]);
					field[LenMACRO-1] = 0;
					user_set_macro(number, field);
					free(field);
				}
				break;
			}
		PRINTF("Expected macro number 0-9\n");
		break;
	default:
		PRINTF("usr_disp_field(%d) ... internal error [token not found]\n",
			token);
		exit(1);
	}

	user_refresh();
	return OK;
}
