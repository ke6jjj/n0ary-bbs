#include <stdio.h>
#include <stdlib.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "msgd.h"

int operation = FALSE;
extern int shutdown_daemon;

static char *
help_disp(void)
{
	output[0] = 0;
	strcat(output, "USER <call>  ...............  set user\n");
	strcat(output, "SYSOP  .....................  set sysop mode\n");
	strcat(output, "NORMAL  ....................  set normal user mode\n");
	strcat(output, "MINE  ......................  set list mine mode\n");
	strcat(output, "BBS  .......................  set bbs2bbs mode\n");
	strcat(output, "SINCE <time>  ..............  set list since mode\n");
	strcat(output, "KILL[H] <number>  ..........  kill message number [Hard]\n");
	strcat(output, "ACTIVE <number>  ...........  activate message number\n");
	strcat(output, "HOLD <number>  .............  hold message number\n");
	strcat(output, "IMMUNE <number>  ...........  make immune message number\n");
	strcat(output, "PARSE <number>  ............  re-parse a message body\n");
	strcat(output, "LIST .......................  list messages, changes\n");
	strcat(output, "READ <number>  .............  read message number\n");
	strcat(output, "READH <number>  ............  read message with headers\n");
	strcat(output, "READRFC <number>  ..........  read rfc822 fields\n");
	strcat(output, "WHO <number>  ..............  show who has read a message\n");
	strcat(output, "WHY <number>  ..............  show why a message was held\n");
	strcat(output, "SEND  ......................  send message\n");
	strcat(output, "FLUSH  .....................  flush previous listing\n");
	strcat(output, "CATCHUP  ...................  catchup listing\n");
	strcat(output, "DISP <mode>  ...............  change disp format\n");
	strcat(output, "   mode = BINARY   (client/server use)\n");
	strcat(output, "          NORMAL   (standard bbs use)\n");
	strcat(output, "          VERBOSE  (human readable form\n");
	strcat(output, "EDIT <number> <rfc822> .....  edit message fields\n");
	strcat(output, "               To: \n");
	strcat(output, "               From: \n");
	strcat(output, "               Subject: \n");
	strcat(output, "               X-Type: \n");
	strcat(output, "               X-Bid: \n");
	strcat(output, "               X-Password: \n");
	strcat(output, "ROUTE <number>  ............  show routing for message\n");
	strcat(output, "ROUTE <address>  ...........  show routing for address\n");
	strcat(output, "PENDING  ...................  show all pending mail\n");
	strcat(output, "PENDING <bbs> [msgtype]  ...  show pending mail for bbs\n");
	strcat(output, "PENDING <number>  ..........  show bbss pending for msg\n");
	strcat(output, "FORWARD <number> <bbs>  ....  message has been forwarded\n");
	strcat(output, "GROUP ......................  show possible groups\n");
	strcat(output, "GROUP <name>  ..............  go to group\n");
	strcat(output, "GROUP OFF  .................  leave group mode\n");
	strcat(output, "REHASH  ....................  reinit message lists\n");
	strcat(output, "AGE  .......................  initiate aging\n");
	strcat(output, "COMPRESS  ..................  compress message database\n");
	strcat(output, "DEBUG LOG [ON|OFF|CLR]  ....  logging options\n");

	strcat(output, ".\n");
	return output;
}

char *
set_parameters(char *s)
{
	char *opt = get_string(&s);
	uppercase(opt);

	if(!strcmp(opt, "LOG")) {
		if(*s && *s != 0) {
			strcpy(opt, get_string(&s));
			uppercase(opt);
			if(!strcmp(opt, "ON"))
				Logging = logON;
			if(!strcmp(opt, "CLR"))
				Logging = logONnCLR;
			if(!strcmp(opt, "OFF"))
				Logging = logOFF;
		}
		switch(Logging) {
		case logOFF:
			return Ok("logging is off");
		case logON:
			return Ok("logging is on");
		case logONnCLR:
			return Ok("logging is on with clearing enabled");
		}
		return Ok("unknown logging status");
	}
	if(!strcmp(opt, "TIME")) {
		time_now = str2time_t(s);
		return "OK\n";
	}

	return Error("invalid DEBUG command");
}

char *
parse(struct active_processes *ap, char *s)
{
	char buf[80];
	struct msg_dir_entry *msg;

	strcpy(buf, get_string(&s));
	uppercase(buf);

	if(buf[0] == '?')
		return help_disp();

	if(!strcmp(buf, "EXIT"))
		return NULL;

	if(!strcmp(buf, "SHUTDOWN")) {
		shutdown_daemon = TRUE;
		return NULL;
	}

	if(!strcmp(buf, "STAT")) {
		return msg_stats();
	}

	if(!strcmp(buf, "DEBUG"))
		return set_parameters(s);

	if(!strcmp(buf, "USER")) {
		strncpy(ap->call, get_string(&s), 20);
		uppercase(ap->call);
		ap->list_sent = FALSE;
		return "OK\n";
	}

	if(!strcmp(buf, "AGE")) {
		age_messages();
		return "OK\n";
	}

	if(!strcmp(buf, "REHASH")) {
		free_msgdir();
		build_msgdir();
		clean_users();
		return "OK\n";
	}

	if(ap->call[0] == 0)
		return Error("Must specify USER");

	if(!strcmp(buf, "FLUSH")) {
		ap->list_sent = FALSE;
		return "OK\n";
	}

	if(!strcmp(buf, "SYSOP")) {
		ap->list_mode = SysopMode;
		ap->list_sent = FALSE;
		return "OK\n";
	}

	if(!strcmp(buf, "BBS")) {
		ap->list_mode = BbsMode;
		ap->list_sent = FALSE;
		return "OK\n";
	}

	if(!strcmp(buf, "NORMAL")) {
		ap->list_mode = NormalMode;
		ap->list_sent = FALSE;
		return "OK\n";
	}

	if(!strcmp(buf, "MINE")) {
		ap->list_mode = MineMode;
		ap->list_sent = FALSE;
		return "OK\n";
	}

	if(!strcmp(buf, "SINCE")) {
		ap->list_mode = get_number(&s);
		ap->list_sent = FALSE;
		return "OK\n";
	}

	if(!strcmp(buf, "DISP")) {
		char buf[20];
		strncpy(buf, get_string(&s), 20);
		uppercase(buf);
		if(!strcmp(buf, "NORMAL"))
			ap->disp_mode = dispNORMAL;
		else if(!strcmp(buf, "VERBOSE"))
			ap->disp_mode = dispVERBOSE;
		else if(!strcmp(buf, "BINARY"))
			ap->disp_mode = dispBINARY;
		else 
			return Error("Expected VERBOSE/NORMAL/BINARY");
		
		return "OK\n";
	}

	if(!strcmp(buf, "EDIT")) {
		if((msg = get_message(get_number(&s))) == NULL)
			return Error("Message not found");
		return edit_message(ap, msg, s);
	}

	if(!strcmp(buf, "READ")) {
		if((msg = get_message(get_number(&s))) == NULL)
			return Error("Message not found");
		read_messages(mREAD, ap, msg, get_string(&s));
		return ".\n";
	}
	if(!strcmp(buf, "READH")) {
		if((msg = get_message(get_number(&s))) == NULL)
			return Error("Message not found");
		read_messages(mREADH, ap, msg, get_string(&s));
		return ".\n";
	}
	if(!strcmp(buf, "READRFC")) {
		if((msg = get_message(get_number(&s))) == NULL)
			return Error("Message not found");
		read_messages_rfc(ap, msg);
		return ".\n";
	}

	if(!strcmp(buf, "ROUTE")) {
		check_route(ap, s);
		return ".\n";
	}

	if(!strcmp(buf, "COMPRESS")) {
		if(compress_messages(ap) != OK)
			return ".\n";

			/* this exit will force a reboot of msgd */
		exit(0);
	}

	if(!strcmp(buf, "SEND")) {
		int num = send_message(ap);
		sprintf(output, "OK, %d\n", num);
		return output;
	}
	if(!strcmp(buf, "LIST")) {
		list_messages(ap);
		return ".\n";
	}

	if(!strcmp(buf, "CATCHUP")) {
		ap->list_sent = Time(NULL);
		return "OK\n";
	}

	if(!strcmp(buf, "PENDING")) {
		if(isdigit(*s))
			pending_fwd_num(ap, get_number(&s));
		else {
			char *call = get_string(&s);
			pending_fwd(ap, call, *s);
		}
		return ".\n";
	}

	if(!strcmp(buf, "FORWARD")) {
		int num = get_number(&s);
		fwddir_kill(num, get_string(&s));
		return "OK\n";
	}

	if(!strcmp(buf, "GROUP")) {
		char *name = get_string(&s);

		if(name == NULL) {
			show_groups(ap);
			return ".\n";
		}

		ap->grp = find_group(name);
		ap->list_sent = FALSE;
		if(ap->grp == NULL)
			return Ok("No Group selected");
		return "OK\n";
	}

	if(!strcmp(buf, "WHY")) {
		if((msg = get_message(get_number(&s))) == NULL)
			return Error("Message not found");
		rfc822_display_held(ap, msg->number);
		return ".\n";
	}
	if(!strcmp(buf, "WHO")) {
		if((msg = get_message(get_number(&s))) == NULL)
			return Error("Message not found");
		read_who(ap, msg);
		return ".\n";
	}

	if(!strcmp(buf, "PARSE")) {
		if((msg = get_message(get_number(&s))) == NULL)
			return Error("Message not found");
		fwddir_kill(msg->number, NULL);
		if(rfc822_decode_fields(msg) != OK)
			return Error("Error while parsing RFC822 fields");
		if(msg->flags & MsgActive)
			set_forwarding(ap, msg, FALSE);
		build_list_text(msg);
		return "OK\n";
	}

	if(!strcmp(buf, "KILL")) {
		if((msg = get_message(get_number(&s))) == NULL)
			return Error("Message not found");
		SetMsgKilled(msg);
		if(IsMsgBulletin(msg)) {
			remove_from_groups(msg);
			set_groups(msg);
		}
		build_list_text(msg);
		return "OK\n";
	}
	if(!strcmp(buf, "KILLH")) {
		if((msg = get_message(get_number(&s))) == NULL)
			return Error("Message not found");
		SetMsgKilled(msg);
		if(IsMsgBulletin(msg)) {
			remove_from_groups(msg);
			set_groups(msg);
		}
		msg_body_kill(msg->number);
		msg = unlink_msg_list(msg);
		return "OK\n";
	}

	if(!strcmp(buf, "ACTIVE")) {
		if((msg = get_message(get_number(&s))) == NULL)
			return Error("Message not found");
		SetMsgActive(msg);
		fwddir_kill(msg->number, NULL);
		if(rfc822_decode_fields(msg) != OK)
			return Error("Error while parsing RFC822 fields");
		set_forwarding(ap, msg, FALSE);
		build_list_text(msg);
		return "OK\n";
	}
	if(!strcmp(buf, "HOLD")) {
		if((msg = get_message(get_number(&s))) == NULL)
			return Error("Message not found");
		SetMsgHeld(msg);
		build_list_text(msg);
		rfc822_append(msg->number, rHELDBY, ap->call);
		while(TRUE) {
			char buf[256];
			if(socket_read_line(ap->fd, buf, 256, 10) == ERROR)
				return Error("Trouble reading");
			if(!strcmp(buf, "."))
				break;
			rfc822_append(msg->number, rHELDWHY, buf);
		}
		return "OK\n";
	}
	if(!strcmp(buf, "IMMUNE")) {
		if((msg = get_message(get_number(&s))) == NULL)
			return Error("Message not found");
		SetMsgImmune(msg);
		build_list_text(msg);
		return "OK\n";
	}

	return Error("Unrecognized command");
}
