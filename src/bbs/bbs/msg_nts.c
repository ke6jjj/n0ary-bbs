#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"
#include "user.h"
#include "tokens.h"
#include "message.h"
#include "msg_nts.h"

/*ARGSUSED*/
int
msg_check_t(struct TOKEN *t)
{
#ifdef NOMSGD
	int msg_list[100];
	int msg_cnt = 0;
	int function = NONE;

	if(ImBBS)
		return ERROR;

	while(t->token != END) {
		switch(t->token) {
		case IN:
		case OUT:
			if(function == NONE) {
				function = t->token;
				break;
			}
			PRINT("Both IN and OUT are not allowed in a single CHECK cmd\n");
			return ERROR;

		case NUMBER:
			if(msg_cnt < 100) {
				msg_list[msg_cnt++] = t->value;
				break;
			}
			PRINT("Number of messages to a CHECK command is limited to 100\n");
			return ERROR;

		default:
			return error(946);
		}
		NEXT(t);
	}

		/* if there is no number operand than assign active */

	if(msg_cnt == 0)
		msg_list[msg_cnt++] = active_message;

		/* use 0 as an end-of-list marker */

	msg_list[msg_cnt] = 0;

	return msg_check(function, msg_list);
#else
	PRINTF("msg_check_t() has not been coded\n");
	return ERROR;
#endif
}

/*ARGSUSED*/
int
msg_check(int func, int *msg_list)
{
#ifdef NOMSGD
	struct message_directory_entry m, *msg = &m;

	while(*msg_list) {
		m.number = *msg_list;
		if(get_msg_directory_entry(&m)) {
			system_msg_number(132, *msg_list++);
			continue;
		}

		if(!IsMsgNTS(msg)) {
			PRINTF("Message #%d is not an NTS message\n", *msg_list++);
			continue;
		}

		switch(func) {
		case IN:
			if(!IsMsgCheckedOut(msg)) {
				PRINTF("Message #%d is not currently checked out\n", *msg_list);
				break;
			}

			if(strcmp(m.deliverer, usercall)) {
				PRINTF("The file was checked out to: %s, ", m.deliverer);
				if(!ImSysop) {
					PRINT("you cannot check the file in\n");
					break;
				} else
					PRINT("as sysop you can continue\n");
			}

			m.flags &= ~MsgCheckedOut;
			m.deliverer[0] = 0;
			put_msg_directory_entry(&m);
			break;

		case OUT:
			if(IsMsgCheckedOut(msg)) {
				PRINTF("Message #%d is currently checked out to: %s\n",
					*msg_list, m.deliverer);
				break;
			}

			m.flags |= MsgCheckedOut;
			strcpy(m.deliverer, usercall);
			put_msg_directory_entry(&m);
			break;
		}

		msg_list++;
	}
	return OK;
#else
	PRINTF("msg_check() has not been coded\n");
	return OK;
#endif
}
