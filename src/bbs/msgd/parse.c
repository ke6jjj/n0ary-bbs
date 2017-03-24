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
	strlcpy(output,
		"USER <call>  ...............  set user\n"
		"SYSOP  .....................  set sysop mode\n"
		"NORMAL  ....................  set normal user mode\n"
		"MINE  ......................  set list mine mode\n"
		"BBS  .......................  set bbs2bbs mode\n"
		"SINCE <time>  ..............  set list since mode\n"
		"KILL[H] <number>  ..........  kill message number [Hard]\n"
		"ACTIVE <number>  ...........  activate message number\n"
		"HOLD <number>  .............  hold message number\n"
		"IMMUNE <number>  ...........  make immune message number\n"
		"PARSE <number>  ............  re-parse a message body\n"
		"LIST .......................  list messages, changes\n"
		"READ <number>  .............  read message number\n"
		"READH <number>  ............  read message with headers\n"
		"READRFC <number>  ..........  read rfc822 fields\n"
		"WHO <number>  ..............  show who has read a message\n"
		"WHY <number>  ..............  show why a message was held\n"
		"SEND  ......................  send message\n"
		"FLUSH  .....................  flush previous listing\n"
		"CATCHUP  ...................  catchup listing\n"
		"DISP <mode>  ...............  change disp format\n"
		"   mode = BINARY   (client/server use)\n"
		"          NORMAL   (standard bbs use)\n"
		"          VERBOSE  (human readable form\n"
		"EDIT <number> <rfc822> .....  edit message fields\n"
		"               To: \n"
		"               From: \n"
		"               Subject: \n"
		"               X-Type: \n"
		"               X-Bid: \n"
		"               X-Password: \n"
		"ROUTE <number>  ............  show routing for message\n"
		"ROUTE <address>  ...........  show routing for address\n"
		"PENDING  ...................  show all pending mail\n"
		"PENDING <bbs> [msgtype]  ...  show pending mail for bbs\n"
		"PENDING <number>  ..........  show bbss pending for msg\n"
		"FORWARD <number> <bbs>  ....  message has been forwarded\n"
		"GROUP ......................  show possible groups\n"
		"GROUP <name>  ..............  go to group\n"
		"GROUP OFF  .................  leave group mode\n"
		"REHASH  ....................  reinit message lists\n"
		"AGE  .......................  initiate aging\n"
		"COMPRESS  ..................  compress message database\n"
		"DEBUG LOG [ON|OFF|CLR]  ....  logging options\n"
		".\n",
	sizeof(output));
	return output;
}

char *
set_parameters(char *s)
{
	char *opt = get_string(&s);
	uppercase(opt);

	if(!strcmp(opt, "LOG")) {
		if(*s && *s != 0) {
			/* Given that the destination of this strcpy()
			 * is inside the source of this strcpy(), it is
			 * intrinsically safe. -JJJ
			 */
			strcpy(opt, get_string(&s)); /* OK */
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

	strlcpy(buf, get_string(&s), sizeof(buf));
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
		strlcpy(ap->call, get_string(&s), sizeof(ap->call));
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
		strlcpy(buf, get_string(&s), sizeof(buf));
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
		snprintf(output, sizeof(output), "OK, %d\n", num);
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
