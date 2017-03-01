#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "user.h"

char *
edit_toggle(struct active_processes *ap, int token, char *s)
{
	struct Ports *port;
	long *toggle;
	char buf[80];
	int i;
	int addrok = TRUE;

	switch(token) {
	case uASCEND:	toggle = &(ap->ul->info.ascending); break;
	case uAPPROVED:	toggle = &(ap->ul->info.approved); break;
	case uHDUPLEX:	toggle = &(ap->ul->info.halfduplex); break;
	case uIMMUNE:	toggle = &(ap->ul->info.immune); break;
	case uLOG:		toggle = &(ap->ul->info.logging); break;
	case uNEWLINE:	toggle = &(ap->ul->info.newline); break;
	case uUPPERCASE: toggle = &(ap->ul->info.uppercase); break;
	case uNONHAM:	toggle = &(ap->ul->info.nonham); break;
	case uREGEXP:	toggle = &(ap->ul->info.regexp); break;
	case uSIGNATURE:toggle = &(ap->ul->info.signature); break;
	case uSYSOP:	toggle = &(ap->ul->info.sysop); break;
	case uVACATION:	toggle = &(ap->ul->info.vacation); break;
	case uEMAILALL:	toggle = &(ap->ul->info.emailall); break;

	case uBBS:
		toggle = &(ap->ul->info.bbs);
		if(operation == uSET) {
			struct Ports *port = ap->ul->info.port;
			ap->ul->info.immune = TRUE;
			ap->ul->info.approved = TRUE;
			ap->ul->info.lines = 0;
			ap->ul->info.macro[0][0] = 0;
			while(port) {
				port->allow = TRUE;
				NEXT(port);
			}
			usrdir_immune(ap->ul->call, TRUE);
		}
		break;

	case uEMAIL:
		toggle = &(ap->ul->info.email);
		if(ap->ul->info.email_addr[0] == 0)
			addrok = FALSE;
		break;
	case uPORT:
		if(*s == 0) {
			output[0] = 0;

			i = 0;
			port = ap->ul->info.port;
			while(port) {
				sprintf(output, "%sPORT%d %s %s %d %d %d\n", output, i++,
					port->name, port->allow ? "YES":"NO",
					port->count, port->firstseen, port->lastseen);
				NEXT(port);
			}
			strcat(output, ".\n");
			return output;
		}

		strcpy(buf, get_string(&s));
		uppercase(buf);

		port = ap->ul->info.port;
		while(port) {
			if(!strcmp(buf, port->name))
				break;
			NEXT(port);
		}

		if(port == NULL)
			return Error("not a valid port name");

		toggle = &(port->allow);
		break;
		
	default:
		return Error("not a toggle field");
	}

	switch(operation) {
	case uCLEAR:
		*toggle = FALSE;
		break;
	case uSET:
		if(!addrok)
			return Error("must set email ADDRESS first");
		*toggle = TRUE;
		break;
	default:
		if(*s == 0)
			return *toggle ? "ON\n" : "OFF\n";

		strcpy(buf, get_string(&s));
		uppercase(buf);

		if(!strcmp(buf, "ON")) {
			if(!addrok)
				return Error("must set email ADDRESS first");
			*toggle = TRUE;
		} else
			if(!strcmp(buf, "OFF"))
				*toggle = FALSE;
			else
				return Error("expected either ON or OFF");
	}

	if(token == uIMMUNE)
		usrdir_immune(ap->ul->call, *toggle);

	ap->ul->condition = DIRTY;
	return "OK\n";
}

char *
edit_list(struct active_processes *ap, int token, char *s)
{
	struct IncludeList **list;
	char *string, buf[256];
	int length, raise_case = FALSE;

	switch(token) {
	case uEXCLUDE:
		list = &(ap->ul->info.Exclude);
		length = LenINCLUDE;
		raise_case = TRUE;
		break;
	case uINCLUDE:
		list = &(ap->ul->info.Include);
		length = LenINCLUDE;
		raise_case = TRUE;
		break;

	default:
		return Error("not a string field");
	}

	switch(operation) {
	case uCLEAR:
		*list = free_list(*list);
		break;
	case uSET:
		return Error("SET not valid to a list field");
	default:
		if(*s == 0) {
			struct IncludeList *t = *list;
			output[0] = 0;
			while(t) {
				sprintf(output, "%s%s\n", output, t->str);
				NEXT(t);
			}
			strcat(output, ".\n");
			return output;
		}

		{
			char *p = (char*)index(s, '\r');
			if(p)
				*p = 0;
		}

		*list = add_2_list(*list, "");
		string = (*list)->str;

		strncpy(buf, s, length);
		strcpy(string, buf);
		if(raise_case == TRUE)
			uppercase(string);
		break;
	}

	ap->ul->condition = DIRTY;
	return "OK\n";
}

char *
edit_string(struct active_processes *ap, int token, char *s)
{
	char *string, buf[256];
	int length, i, raise_case = FALSE;

	switch(token) {
	case uADDRESS:
		string = ap->ul->info.email_addr;
		length = LenEMAIL;
		break;
	case uCOMPUTER:
		string = ap->ul->info.computer;
		length = LenEQUIP;
		break;
	case uFREQ:
		string = ap->ul->info.freq;
		length = LenFREQ;
		break;
	case uLNAME:
		string = ap->ul->info.lname;
		length = LenLNAME;
		break;
	case uPHONE:
		string = ap->ul->info.phone;
		length = LenPHONE;
		break;
	case uRIG:
		string = ap->ul->info.rig;
		length = LenEQUIP;
		break;
	case uSOFTWARE:
		string = ap->ul->info.software;
		length = LenEQUIP;
		break;
	case uTNC:
		string = ap->ul->info.tnc;
		length = LenEQUIP;
		break;
	case uMACRO:
		if(!isdigit(*s))
			return Error("expected a macro number");
		i = get_number(&s);
		if(i<0 || i>9)
			return Error("expected a macro number between 0 and 9, inclusive");
		string = ap->ul->info.macro[i];
		length = LenMACRO;
		break;

	default:
		return Error("not a string field");
	}

	switch(operation) {
	case uCLEAR:
		*string = 0;
		if(token == uADDRESS)
			ap->ul->info.email = FALSE;
		break;
	case uSET:
		return Error("SET not valid to a string field");
	default:
		if(*s == 0) {
			sprintf(output, "%s\n", string);
			return output;
		}

		{
			char *p = (char*)index(s, '\r');
			if(p)
				*p = 0;
		}
		strncpy(buf, s, length);
		strcpy(string, buf);
		if(raise_case == TRUE)
			uppercase(string);
		break;
	}

	ap->ul->condition = DIRTY;
	return "OK\n";
}

char *
edit_number(struct active_processes *ap, int token, char *s)
{
	long *number; 
	struct Ports *port;
	long cnt = 0;

	if(operation)
		return Error("illegal operand to CLEAR/SET command");

	switch(token) {
	case uHELP:		number = &(ap->ul->info.help); break;
	case uLINES:	number = &(ap->ul->info.lines); break;
	case uBASE:		number = &(ap->ul->info.base); break;
	case uMESSAGE:	number = &(ap->ul->info.message); break;
	case uNUMBER:	number = &(ap->ul->info.number); break;
	case uCOUNT:
		port = ap->ul->info.port;
		while(port) {
			cnt += port->count;
			NEXT(port);
		}
		number = &cnt;
		break;
	default:
		return Error("not a number field");
	}

	if(*s == 0) {
		sprintf(output, "%d\n", *number);
		return output;
	}

	if(!isdigit(*s))
		return Error("expected a number operand");

	*number = get_number(&s);

	if(token == uNUMBER)
		usrdir_number(ap->ul->call, *number);

	ap->ul->condition = DIRTY;
	return "OK\n";
}

char *
show_user(struct active_processes *ap)
{
	int i;
	struct Ports *port;

	if(ap->ul == NULL)
		return Error("must specify a user");

	output[0] = 0;
	if(ap->ul->info.lname[0])
		sprintf(output, "%sLNAME %s\n", output, ap->ul->info.lname);
	if(ap->ul->info.phone[0])
		sprintf(output, "%sPHONE %s\n", output, ap->ul->info.phone);
	if(ap->ul->info.email_addr[0])
		sprintf(output, "%sADDRESS %s\n", output, ap->ul->info.email_addr);
	if(ap->ul->info.freq[0])
		sprintf(output, "%sFREQ %s\n", output, ap->ul->info.freq);
	if(ap->ul->info.tnc[0])
		sprintf(output, "%sTNC %s\n", output, ap->ul->info.tnc);
	if(ap->ul->info.software[0])
		sprintf(output, "%sSOFTWARE %s\n", output, ap->ul->info.software);
	if(ap->ul->info.computer[0])
		sprintf(output, "%sCOMPUTER %s\n", output, ap->ul->info.computer);
	if(ap->ul->info.rig[0])
		sprintf(output, "%sRIG %s\n", output, ap->ul->info.rig);

	if(ap->ul->info.base)
		sprintf(output, "%sBASE %d\n", output, ap->ul->info.base);
	if(ap->ul->info.lines)
		sprintf(output, "%sLINES %d\n", output, ap->ul->info.lines);
	if(ap->ul->info.message)
		sprintf(output, "%sMESSAGE %d\n", output, ap->ul->info.message);

	sprintf(output, "%sNUMBER %d\n", output, ap->ul->info.number);

	if(ap->ul->info.sysop) strcat(output, "SYSOP\n");
	if(ap->ul->info.bbs) strcat(output, "BBS\n");
	if(ap->ul->info.approved) strcat(output, "APPROVED\n");
	if(ap->ul->info.nonham) strcat(output, "NONHAM\n");
	if(ap->ul->info.vacation) strcat(output, "VACATION\n");
	if(ap->ul->info.signature) strcat(output, "SIGNATURE\n");
	if(ap->ul->info.logging) strcat(output, "LOGGING\n");
	if(ap->ul->info.immune) strcat(output, "IMMUNE\n");
	if(ap->ul->info.regexp) strcat(output, "REGEXP\n");
	if(ap->ul->info.halfduplex) strcat(output, "DUPLEX\n");
	if(ap->ul->info.newline) strcat(output, "NEWLINE\n");
	if(ap->ul->info.uppercase) strcat(output, "UPPERCASE\n");
	if(ap->ul->info.ascending) strcat(output, "LIST ASCEND\n");
	if(ap->ul->info.email) strcat(output, "EMAIL\n");
	if(ap->ul->info.emailall) strcat(output, "EMAILALL\n");

	if(ap->ul->info.help) sprintf(output, "%sHELP %d\n", output, ap->ul->info.help);

	for(i=0; i<10; i++)
		if(ap->ul->info.macro[i][0])
			sprintf(output, "%sMACRO %d %s\n", output, i, ap->ul->info.macro[i]);

	if(ap->ul->info.Include) {
		struct IncludeList *list = ap->ul->info.Include;
		while(list) {
			sprintf(output, "%sINCLUDE %s\n", output, list->str);
			NEXT(list);
		}
	}

	if(ap->ul->info.Exclude) {
		struct IncludeList *list = ap->ul->info.Exclude;
		while(list) {
			sprintf(output, "%sEXCLUDE %s\n", output, list->str);
			NEXT(list);
		}
	}

	port = ap->ul->info.port;
	while(port) {
		sprintf(output, "%sPORT %s %s %d %d %d\n", output,
			port->name, port->allow ? "YES":"NO",
			port->count, port->firstseen, port->lastseen);
		NEXT(port);
	}

	strcat(output, ".\n");
	return output;
}

char *
create_user(struct active_processes *ap, char *s)
{
	char call[10];

	if(*s == 0)
		return Error("expected callsign");

	strcpy(call, get_string(&s));
	uppercase(call);
	if(strlen(call) > 6)
		return Error("call too long");

	if(usrfile_create(call) == OK)
		usrfile_read(call, ap);

	return "OK\n";
}	

char *
login_user(struct active_processes *ap, char *s)
{
	char name[10];
	struct Ports *port = ap->ul->info.port;
	int i = 0;

	strcpy(name, get_string(&s));

	while(port) {
		if(!strcmp(port->name, name)) {
			time_t now = Time(NULL);

			if(port->allow == FALSE)
				return Error("not authorized for access via this port");

			port->lastseen = now;
			if(port->firstseen == 0)
				port->firstseen = now;
			port->count++;
			ap->ul->condition = DIRTY;

			usrdir_touch(ap->ul->call, i, now);
			return "OK\n";
		}
		i++;
		NEXT(port);
	}

	return Error("Invalid port name");
}	

char *
kill_user(char *s)
{
	char fn[80], call[10];

	if(*s == 0)
		return Error("Expected a callsign");

	while(*s) {
		strncpy(call, get_string(&s), 10);
		uppercase(call);

		sprintf(fn, "%s/%s", Userd_Acc_Path, call);

		if(usrdir_kill(call) == OK)
			if(usrfile_kill(call) == OK)
				unlink(fn);
	}
	return "OK\n";
}
