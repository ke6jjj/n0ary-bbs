#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#ifdef SUNOS
#include <termio.h>
#else
#include <termios.h>
#include <sys/ioctl.h>
#endif

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "vars.h"
#include "user.h"
#include "tokens.h"
#include "message.h"
#include "function.h"
#include "maintenance.h"
#include "bbscommon.h"

static int echo(int cond);

time_t
	time_now;

int
	debug_level = DBG_OFF,
	dbug_old_level = DBG_OFF;

/* This is the sysop master account setup. When all else fails a bbs owner
 * can come in through this account to bring the baby back up. Otherwise
 * there is no way to reach sysop access.
 */

void
check_for_root_access(void)
{
	/* For this to work the user must be named SYSOP */
	if(strcmp(usercall, "SYSOP"))
		return;

	/* The entry mode must be from the console also. There is a way to fool
	   the code by coming in on the phone line into a non-bbs account and
	   entering the bbs from unix, but that is ok I guess. */

	if(port_type(Via) != tCONSOLE)
		return;

	/* Ok now set the appropriate bits to allow this user to use the sysop
	   command. He will still have to know the sysop password. */

	user_set_flag(uAPPROVED);
	user_set_flag(uSYSOP);
	user_set_flag(uLOG);

	if(match_sysop_password() == OK)
		ImSysop = TRUE;
}

void
display_debug_usage(void)
{
	PRINTF("DEBUG xxxx\n");
	PRINTF("      TOKENS      Display tokenized command\n");
	PRINTF("      TRANS       Display message translation\n");
	PRINTF("      LIST        Display message list criteria\n");
	PRINTF("      FWD         Display forwarding determination\n");
	PRINTF("      NEW         Work on new code\n");
	PRINTF("      USER        Display user flag transitions\n\n");
	PRINTF("      ON          Pop Debug level\n");
	PRINTF("      OFF         Push Debug level and clear\n\n");
}

void
display_debug_level(void)
{
	PRINTF("Debug: ");
	if(debug_level == 0) {
		PRINTF("Off\n");
		return;
	}

	if(debug_level & DBG_TOKENS)
		PRINTF("TOKENS ");
	if(debug_level & DBG_MSGTRANS)
		PRINTF("TRANS ");
	if(debug_level & DBG_MSGFWD)
		PRINTF("FWD ");
	if(debug_level & DBG_MSGLIST)
		PRINTF("LIST ");
	if(debug_level & DBG_USER)
		PRINTF("USER ");
	if(debug_level & DBG_NEW)
		PRINTF("NEW ");

	PRINTF("\n");
}

int
maint(void)
{
	struct TOKEN *t = TokenList;
	char buf[80];
	int tmp;

	switch(t->token) {
	case BYE:
		if((t->next->token != HARD) && ImSysop) {
			bbsd_msg("Exiting sysop mode");
			ImSysop = FALSE;
			flush_message_list();
			msg_NormalMode();
			return OK;
		}
		exit_bbs();

	case VERSION:
		PRINTF("N0ARY/BBS Version %s\n", build_sid());
		return OK;

	case SYSOP:
		flush_message_list();
		if(t->next->token == HARD && ImAllowedSysop)
			ImSysop = TRUE;
		else {
			if(ImSysop)
				PRINT("Already in sysop mode, pay attention\n");
			else {
				if(match_sysop_password() == OK)
					ImSysop = TRUE;
			    else
					check_for_root_access();
			}
		}
		if(ImSysop)
			msg_SysopMode();
		break;

	case DEBUG:
		NEXT(t);
		if(t->token == END) {
			display_debug_usage();
			display_debug_level();
			return OK;
		}

		while(t->token != END) {
			switch(t->token) {
			case TIME:
				NEXT(t);
				if(t->token == STRING)
					time_now = str2time_t(t->lexem);
				else {
					struct tm *dt = localtime(&time_now);
					char datebuf[40];
					strftime(datebuf, 40, "%Y-%m-%d %R", dt);
					PRINT(datebuf);
				}
				return OK;

			case DBG_MSGLIST:
				msgd_debug(1);

			case DBG_TOKENS:
			case DBG_MSGFWD:
			case DBG_MSGTRANS:
			case DBG_USER:
			case DBG_NEW:
				debug_level |= t->token;
				break;
			case OFF:
				dbug_old_level = debug_level;
				msgd_debug(0);
				debug_level = DBG_OFF;
				break;
			case ON:
				debug_level = dbug_old_level;
				if(debug_level & DBG_MSGLIST)
					msgd_debug(1);
				display_debug_level();
				break;
			default:
				PRINTF("Unknown token %s\n", t->lexem);
				break;
			}
			NEXT(t);
		}
		break;
	}

	return OK;
}

int
echo_off(void)
{
	return echo(OFF);
}

static int
echo(int cond)
{
	struct termios tt;

#ifdef HAVE_TERMIOS
	if (tcgetattr(0, &tt))
		return error_log("echo.tcgetattr: %s", sys_errlist[errno]);
#else
#error "Need termios"
#endif

	switch(cond) {
	case ON:
		tt.c_lflag |= ECHO;
		break;
	case OFF:
		tt.c_lflag &= ~ECHO;
		break;
	}

#ifdef HAVE_TERMIOS
	if(tcsetattr(0, TCSANOW, &tt))
		return error_log("echo.tcsetattr: %s", sys_errlist[errno]);
#else
#error "Need termios"
#endif

	return OK;
}
