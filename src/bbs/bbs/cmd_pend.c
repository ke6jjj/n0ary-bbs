#include <stdio.h>
#include <string.h>

#include "c_cmmn.h"
#include "tools.h"
#include "config.h"
#include "function.h"
#include "pending.h"
#include "tokens.h"
#include "bbslib.h"
#include "message.h"

int
	PendingCmd = FALSE;

struct pending_operations
	*PendingOp = NULL;

extern void 
	init_server(char *server, int msg_number, char *text);

struct pending_operations *
queue_pending_operation(int cmd)
{
	struct pending_operations 
		*po = malloc_struct(pending_operations),
		*lpo = PendingOp;

	if(po == NULL) {
		PRINT("Memory Allocation Failure, exiting\n");
		exit(1);
	}

	if(lpo == NULL)
		PendingOp = po;
	else {
		while(lpo->next != NULL)
			NEXT(lpo);
		lpo->next = po;
	}

	po->cmd = cmd;
	return po;
}

void
queue_pending_command(char *cmd)
{
	struct pending_operations *po = queue_pending_operation(COMMANDS);
	strcpy(po->text, cmd);
}

run_pending_operations(void)
{
	struct pending_operations *lpo, *po = PendingOp;

	PendingCmd = TRUE;

	while(po) {
			/* get an up-to-date listing. If this is followup to a SEND
			 * command we will be out of sync.
			 */
		build_full_message_list();

		if(po->number == ERROR)
			po->number = active_message;
		switch(po->cmd) {
		case MAIL:
			msg_mail(po->number, po->name);
			break;
		case HOLD:
			msg_hold_by_bbs(po->number, po->text);
			break;
		case COPY:
			msg_copy_parse(po->number, po->name);
			break;
#if 0
		case VACATION:
			InServer = 'V';
			msg_rply(po->number);
			InServer = FALSE;
			break;
#endif
		case INITIATE:
			init_server(po->name, po->number, po->text);
			break;
		case WRITE:
			filesys_write_msg(po->number, po->name);
			break;
		case COMMANDS:
			PRINTF("\nExecute: {%s} (Y/n)\n", po->text);
			if(get_yes_no_quit(YES) == YES)
				parse_command_line(po->text);	
			break;
		default:
			PRINT("run_pending_operations: Command not supported\n");
		}

		lpo = po;
		NEXT(po);
		free(lpo);	
	}
	PendingCmd = FALSE;
	PendingOp = NULL;
}
