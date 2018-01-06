#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "bbslib.h"
#include "tokens.h"
#include "function.h"

void
display_tokens(void)
{
	struct TOKEN *t = TokenList;

	while(t) {
		switch(t->token) {
		case ABOUT:
			PRINTF("  ABOUT\t[%s]\n", t->lexem);
			break;
		case ACTIVATE:
			PRINTF("  ACTIVATE\t[%s]\n", t->lexem);
			break;
		case ASCEND:
			PRINTF("  ASCEND\t[%s]\n", t->lexem);
			break;
		case ADD:
			PRINTF("  ADD\t[%s]\n", t->lexem);
			break;
		case ADDRESS:
			PRINTF("  ADDRESS\t[%s]\n", t->lexem);
			break;
		case AGE:
			PRINTF("  AGE\t[%s]\n", t->lexem);
			break;
		case ALL:
			PRINTF("  ALL\t[%s]\n", t->lexem);
			break;
		case ALTERED:
			PRINTF("  ALTERED\t[%s]\n", t->lexem);
			break;
		case APPROVED:
			PRINTF("  APPROVED\t[%s]\n", t->lexem);
			break;
		case AT:
			PRINTF("  AT\t[%s]\n", t->lexem);
			break;
		case ATBBS:
			PRINTF("  ATBBS\t[%s]\n", t->lexem);
			break;
		case ATDIST:
			PRINTF("  ATDIST\t[%s]\n", t->lexem);
			break;
		case ATNTS:
			PRINTF("  ATNTS\t[%s]\n", t->lexem);
			break;
		case BACKSLASH:
			PRINTF("  BACKSLASH\t[%s]\n", t->lexem);
			break;
		case BAROMETER:
			PRINTF("  BAROMETER\t[%s]\n", t->lexem);
			break;
		case BBS:
			PRINTF("  BBS\t[%s]\n", t->lexem);
			break;
		case BBSSID:
			PRINTF("  BBSSID\t[%s]\n", t->lexem);
			break;
		case BID:
			PRINTF("  BID\t[%s]\n", t->lexem);
			break;
		case BREAK:
			PRINTF("  BREAK\t[%s]\n", t->lexem);
			break;
		case BULLETIN:
			PRINTF("  BULLETIN\t[%s]\n", t->lexem);
			break;
		case BYE:
			PRINTF("  BYE\t[%s]\n", t->lexem);
			break;
		case CALL:
			PRINTF("  CALL\t[%s]\n", t->lexem);
			break;
		case CALLBK:
			PRINTF("  CALLBK\t[%s]\n", t->lexem);
			break;
		case CATCHUP:
			PRINTF("  CATCHUP\t[%s]\n", t->lexem);
			break;
		case CD:
			PRINTF("  CD\t[%s]\n", t->lexem);
			break;
		case CHANGE:
			PRINTF("  CHANGE\t[%s]\n", t->lexem);
			break;
		case CHECK:
			PRINTF("  CHECK\t[%s]\n", t->lexem);
			break;
		case CLEAR:
			PRINTF("  CLEAR\t[%s]\n", t->lexem);
			break;
		case COMMA:
			PRINTF("  COMMA\t[%s]\n", t->lexem);
			break;
		case COMMANDS:
			PRINTF("  COMMANDS\t[%s]\n", t->lexem);
			break;
		case COMPRESS:
			PRINTF("  COMPRESS\t[%s]\n", t->lexem);
			break;
		case COMPUTER:
			PRINTF("  COMPUTER\t[%s]\n", t->lexem);
			break;
		case CONSOLE:
			PRINTF("  CONSOLE\t[%s]\n", t->lexem);
			break;
		case COUNT:
			PRINTF("  COUNT\t[%s]\n", t->lexem);
			break;
		case COPY:
			PRINTF("  COPY\t[%s]\n", t->lexem);
			break;
		case CUSTOM:
			PRINTF("  CUSTOM\t[%s]\n", t->lexem);
			break;
		case DAYS:
			PRINTF("  DAYS\t[%s]\n", t->lexem);
			break;
		case DASH:
			PRINTF("  DASH\t[%s]\n", t->lexem);
			break;
		case DATA:
			PRINTF("  DATA\t[%s]\n", t->lexem);
			break;
		case DATE:
			PRINTF("  DATE\t[%s]\n", t->lexem);
			break;
		case DELETE:
			PRINTF("  DELETE\t[%s]\n", t->lexem);
			break;
		case DESCEND:
			PRINTF("  DESCEND\t[%s]\n", t->lexem);
			break;
		case DIRECTORY:
			PRINTF("  DIRECTORY\t[%s]\n", t->lexem);
			break;
		case DISTRIB:
			PRINTF("  DISTRIB\t[%s]\n", t->lexem);
			break;
		case DUMP:
			PRINTF("  DUMP\t[%s]\n", t->lexem);
			break;
		case EDIT:
			PRINTF("  EDIT\t[%s]\n", t->lexem);
			break;
		case EQUIP:
			PRINTF("  EQUIP\t[%s]\n", t->lexem);
			break;
		case EVENT:
			PRINTF("  EVENT\t[%s]\n", t->lexem);
			break;
		case EXCLUDE:
			PRINTF("  EXCLUDE\t[%s]\n", t->lexem);
			break;
		case EXPIRED:
			PRINTF("  EXPIRED\t[%s]\n", t->lexem);
			break;
		case FAST:
			PRINTF("  FAST\t[%s]\n", t->lexem);
			break;
		case FILESYS:
			PRINTF("  FILESYS\t[%s]\n", t->lexem);
			break;
		case FIRST:
			PRINTF("  FIRST\t[%s]\n", t->lexem);
			break;
		case FIX	:
			PRINTF("  FIX	\t[%s]\n", t->lexem);
			break;
		case FNAME:
			PRINTF("  FNAME\t[%s]\n", t->lexem);
			break;
		case FORWARD:
			PRINTF("  FORWARD\t[%s]\n", t->lexem);
			break;
		case FREQ:
			PRINTF("  FREQ\t[%s]\n", t->lexem);
			break;
		case FRIDAY:
			PRINTF("  FRIDAY\t[%s]\n", t->lexem);
			break;
		case FROM:
			PRINTF("  FROM\t[%s]\n", t->lexem);
			break;
		case FULL:
			PRINTF("  FULL\t[%s]\n", t->lexem);
			break;
		case GENERATE:
			PRINTF("  GENERATE\t[%s]\n", t->lexem);
			break;
		case GRAPH:
			PRINTF("  GRAPH\t[%s]\n", t->lexem);
			break;
		case HARD:
			PRINTF("  HARD\t[%s]\n", t->lexem);
			break;
		case HDX:
			PRINTF("  HDX\t[%s]\n", t->lexem);
			break;
		case HEADER:
			PRINTF("  HEADER\t[%s]\n", t->lexem);
			break;
		case HELD:
			PRINTF("  HELD\t[%s]\n", t->lexem);
			break;
		case HELP:
			PRINTF("  HELP\t[%s]\n", t->lexem);
			break;
		case HLOC:
			PRINTF("  HLOC\t[%s]\n", t->lexem);
			break;
		case HOLD:
			PRINTF("  HOLD\t[%s]\n", t->lexem);
			break;
		case HOME:
			PRINTF("  HOME\t[%s]\n", t->lexem);
			break;
		case HUMIDITY:
			PRINTF("  HUMIDITY\t[%s]\n", t->lexem);
			break;
		case IMMUNE:
			PRINTF("  IMMUNE\t[%s]\n", t->lexem);
			break;
		case INCLUDE:
			PRINTF("  INCLUDE\t[%s]\n", t->lexem);
			break;
		case INFO:
			PRINTF("  INFO\t[%s]\n", t->lexem);
			break;
		case INITIATE:
			PRINTF("  INITIATE\t[%s]\n", t->lexem);
			break;
		case IN:
			PRINTF("  IN\t[%s]\n", t->lexem);
			break;
		case INDOOR:
			PRINTF("  INDOOR\t[%s]\n", t->lexem);
			break;
		case KILL:
			PRINTF("  KILL\t[%s]\n", t->lexem);
			break;
		case LAST:
			PRINTF("  LAST\t[%s]\n", t->lexem);
			break;
		case LINES:
			PRINTF("  LINES\t[%s]\n", t->lexem);
			break;
		case LIST:
			PRINTF("  LIST\t[%s]\n", t->lexem);
			break;
		case LNAME:
			PRINTF("  LNAME\t[%s]\n", t->lexem);
			break;
		case LOAD:
			PRINTF("  LOAD\t[%s]\n", t->lexem);
			break;
		case LOCAL:
			PRINTF("  LOCAL\t[%s]\n", t->lexem);
			break;
		case LOCALE:
			PRINTF("  LOCALE\t[%s]\n", t->lexem);
			break;
		case LOCK:
			PRINTF("  LOCK\t[%s]\n", t->lexem);
			break;
		case LOG:
			PRINTF("  LOG\t[%s]\n", t->lexem);
			break;
		case LOOKUP:
			PRINTF("  LOOKUP\t[%s]\n", t->lexem);
			break;
		case LS:
			PRINTF("  LS\t[%s]\n", t->lexem);
			break;
		case MACRO:
			PRINTF("  MACRO\t[%s]\n", t->lexem);
			break;
		case MAIL:
			PRINTF("  MAIL\t[%s]\n", t->lexem);
			break;
		case MAINT:
			PRINTF("  MAINT\t[%s]\n", t->lexem);
			break;
		case ME:
			PRINTF("  ME\t[%s]\n", t->lexem);
			break;
		case MESSAGE:
			PRINTF("  MESSAGE\t[%s]\n", t->lexem);
			break;
		case MINE:
			PRINTF("  MINE\t[%s]\n", t->lexem);
			break;
		case MONDAY:
			PRINTF("  MONDAY\t[%s]\n", t->lexem);
			break;
		case MONTHS:
			PRINTF("  MONTHS\t[%s]\n", t->lexem);
			break;
		case MOTD:
			PRINTF("  MOTD\t[%s]\n", t->lexem);
			break;
		case NEW:
			PRINTF("  NEW\t[%s]\n", t->lexem);
			break;
		case NEWLINE:
			PRINTF("  NEWLINE\t[%s]\n", t->lexem);
			break;
		case NONE:
			PRINTF("  NONE\t[%s]\n", t->lexem);
			break;
		case NONHAM:
			PRINTF("  NONHAM\t[%s]\n", t->lexem);
			break;
		case NOPROMPT:
			PRINTF("  NOPROMPT\t[%s]\n", t->lexem);
			break;
		case NTS:
			PRINTF("  NTS\t[%s]\n", t->lexem);
			break;
		case NUMBER:
			PRINTF("  NUMBER\t[%s]\n", t->lexem);
			break;
		case OFF:
			PRINTF("  OFF\t[%s]\n", t->lexem);
			break;
		case OLD:
			PRINTF("  OLD\t[%s]\n", t->lexem);
			break;
		case ON:
			PRINTF("  ON\t[%s]\n", t->lexem);
			break;
		case OUT:
			PRINTF("  OUT\t[%s]\n", t->lexem);
			break;
		case PASSWORD:
			PRINTF("  PASSWORD\t[%s]\n", t->lexem);
			break;
		case PENDING:
			PRINTF("  PENDING\t[%s]\n", t->lexem);
			break;
		case PERSONAL:
			PRINTF("  PERSONAL\t[%s]\n", t->lexem);
			break;
		case PERIOD:
			PRINTF("  PERIOD\t[%s]\n", t->lexem);
			break;
		case PHONE:
			PRINTF("  PHONE\t[%s]\n", t->lexem);
			break;
		case PORTS:
			PRINTF("  PORTS\t[%s]\n", t->lexem);
			break;
		case PROCESS:
			PRINTF("  PROCESS\t[%s]\n", t->lexem);
			break;
		case PUNCTUATION:
			PRINTF("  PUNCTUATION\t[%s]\n", t->lexem);
			break;
		case QTH:
			PRINTF("  QTH\t[%s]\n", t->lexem);
			break;
		case RAIN:
			PRINTF("  RAIN\t[%s]\n", t->lexem);
			break;
		case READ:
			PRINTF("  READ\t[%s]\n", t->lexem);
			break;
		case REFRESH:
			PRINTF("  REFRESH\t[%s]\n", t->lexem);
			break;
		case REGEXP:
			PRINTF("  REGEXP\t[%s]\n", t->lexem);
			break;
		case RELEASE:
			PRINTF("  RELEASE\t[%s]\n", t->lexem);
			break;
		case REMOTE:
			PRINTF("  REMOTE\t[%s]\n", t->lexem);
			break;
		case REPLY:
			PRINTF("  REPLY\t[%s]\n", t->lexem);
			break;
		case RESTRICTED:
			PRINTF("  RESTRICTED\t[%s]\n", t->lexem);
			break;
		case REVFWD:
			PRINTF("  REVFWD\t[%s]\n", t->lexem);
			break;
		case RIG:
			PRINTF("  RIG\t[%s]\n", t->lexem);
			break;
		case SATURDAY:
			PRINTF("  SATURDAY\t[%s]\n", t->lexem);
			break;
		case SEARCH:
			PRINTF("  SEARCH\t[%s]\n", t->lexem);
			break;
		case SECURE:
			PRINTF("  SECURE\t[%s]\n", t->lexem);
			break;
		case SEND:
			PRINTF("  SEND\t[%s]\n", t->lexem);
			break;
		case SET:
			PRINTF("  SET\t[%s]\n", t->lexem);
			break;
		case SHELL:
			PRINTF("  SHELL\t[%s]\n", t->lexem);
			break;
		case SHOW:
			PRINTF("  SHOW\t[%s]\n", t->lexem);
			break;
		case SIGNATURE:
			PRINTF("  SIGNATURE\t[%s]\n", t->lexem);
			break;
		case SINCE:
			PRINTF("  SINCE\t[%s]\n", t->lexem);
			break;
		case SIZE:
			PRINTF("  SIZE\t[%s]\n", t->lexem);
			break;
		case SLASH:
			PRINTF("  SLASH\t[%s]\n", t->lexem);
			break;
		case SLOW:
			PRINTF("  SLOW\t[%s]\n", t->lexem);
			break;
		case SOFTWARE:
			PRINTF("  SOFTWARE\t[%s]\n", t->lexem);
			break;
		case SOLAR:
			PRINTF("  SOLAR\t[%s]\n", t->lexem);
			break;
		case SPAWN:
			PRINTF("  SPAWN\t[%s]\n", t->lexem);
			break;
		case SSID:
			PRINTF("  SSID\t[%s]\n", t->lexem);
			break;
		case SUBJECT:
			PRINTF("  SUBJECT\t[%s]\n", t->lexem);
			break;
		case SUNDAY:
			PRINTF("  SUNDAY\t[%s]\n", t->lexem);
			break;
		case SUSPECT:
			PRINTF("  SUSPECT\t[%s]\n", t->lexem);
			break;
		case SYSOP:
			PRINTF("  SYSOP\t[%s]\n", t->lexem);
			break;
		case STATUS:
			PRINTF("  STATUS\t[%s]\n", t->lexem);
			break;
		case STRING:
			PRINTF("  STRING\t[%s]\n", t->lexem);
			break;
		case TEMPERATURE:
			PRINTF("  TEMPERATURE\t[%s]\n", t->lexem);
			break;
		case THURSDAY:
			PRINTF("  THURSDAY\t[%s]\n", t->lexem);
			break;
		case TIME:
			PRINTF("  TIME\t[%s]\n", t->lexem);
			break;
		case TIMER:
			PRINTF("  TIMER\t[%s]\n", t->lexem);
			break;
		case TO:
			PRINTF("  TO\t[%s]\n", t->lexem);
			break;
		case TOCALL:
			PRINTF("  TOCALL\t[%s]\n", t->lexem);
			break;
		case TOGROUP:
			PRINTF("  TOGROUP\t[%s]\n", t->lexem);
			break;
		case TONTS:
			PRINTF("  TONTS\t[%s]\n", t->lexem);
			break;
		case TOUCH:
			PRINTF("  TOUCH\t[%s]\n", t->lexem);
			break;
		case TNC:
			PRINTF("  TNC\t[%s]\n", t->lexem);
			break;
		case TNC144:
			PRINTF("  TNC144\t[%s]\n", t->lexem);
			break;
		case TNC220:
			PRINTF("  TNC220\t[%s]\n", t->lexem);
			break;
		case TNC440:
			PRINTF("  TNC440\t[%s]\n", t->lexem);
			break;
		case TUESDAY:
			PRINTF("  TUESDAY\t[%s]\n", t->lexem);
			break;
		case UNLOCK:
			PRINTF("  UNLOCK\t[%s]\n", t->lexem);
			break;
		case UNREAD:
			PRINTF("  UNREAD\t[%s]\n", t->lexem);
			break;
		case USER:
			PRINTF("  USER\t[%s]\n", t->lexem);
			break;
		case EMAIL:
			PRINTF("  EMAIL\t[%s]\n", t->lexem);
			break;
		case UUSTAT:
			PRINTF("  UUSTAT\t[%s]\n", t->lexem);
			break;
		case UV:
			PRINTF("  UV\t[%s]\n", t->lexem);
			break;
		case VACATION:
			PRINTF("  VACATION\t[%s]\n", t->lexem);
			break;
		case VOICE:
			PRINTF("  VOICE\t[%s]\n", t->lexem);
			break;
		case WAIT:
			PRINTF("  WAIT\t[%s]\n", t->lexem);
			break;
		case WEDNESDAY:
			PRINTF("  WEDNESDAY\t[%s]\n", t->lexem);
			break;
		case WEEKS:
			PRINTF("  WEEKS\t[%s]\n", t->lexem);
			break;
		case WHEREIS:
			PRINTF("  WHEREIS\t[%s]\n", t->lexem);
			break;
		case WHO:
			PRINTF("  WHO\t[%s]\n", t->lexem);
			break;
		case WIND:
			PRINTF("  WIND\t[%s]\n", t->lexem);
			break;
		case WORD:
			PRINTF("  WORD\t[%s]\n", t->lexem);
			break;
		case WP:
			PRINTF("  WP\t[%s]\n", t->lexem);
			break;
		case WRITE:
			PRINTF("  WRITE\t[%s]\n", t->lexem);
			break;
		case WX:
			PRINTF("  WX\t[%s]\n", t->lexem);
			break;
		case YESTERDAY:
			PRINTF("  YESTERDAY\t[%s]\n", t->lexem);
			break;
		case ZIP:
			PRINTF("  ZIP\t[%s]\n", t->lexem);
			break;
		case ACMD:
			PRINTF("  ACMD\t[%s]\n", t->lexem);
			break;
		case BCMD:
			PRINTF("  BCMD\t[%s]\n", t->lexem);
			break;
		case CCMD:
			PRINTF("  CCMD\t[%s]\n", t->lexem);
			break;
		case DCMD:
			PRINTF("  DCMD\t[%s]\n", t->lexem);
			break;
		case ECMD:
			PRINTF("  ECMD\t[%s]\n", t->lexem);
			break;
		case FCMD:
			PRINTF("  FCMD\t[%s]\n", t->lexem);
			break;
		case GCMD:
			PRINTF("  GCMD\t[%s]\n", t->lexem);
			break;
		case HCMD:
			PRINTF("  HCMD\t[%s]\n", t->lexem);
			break;
		case ICMD:
			PRINTF("  ICMD\t[%s]\n", t->lexem);
			break;
		case JCMD:
			PRINTF("  JCMD\t[%s]\n", t->lexem);
			break;
		case KCMD:
			PRINTF("  KCMD\t[%s]\n", t->lexem);
			break;
		case LCMD:
			PRINTF("  LCMD\t[%s]\n", t->lexem);
			break;
		case MCMD:
			PRINTF("  MCMD\t[%s]\n", t->lexem);
			break;
		case NCMD:
			PRINTF("  NCMD\t[%s]\n", t->lexem);
			break;
		case OCMD:
			PRINTF("  OCMD\t[%s]\n", t->lexem);
			break;
		case PCMD:
			PRINTF("  PCMD\t[%s]\n", t->lexem);
			break;
		case QCMD:
			PRINTF("  QCMD\t[%s]\n", t->lexem);
			break;
		case RCMD:
			PRINTF("  RCMD\t[%s]\n", t->lexem);
			break;
		case SCMD:
			PRINTF("  SCMD\t[%s]\n", t->lexem);
			break;
		case TCMD:
			PRINTF("  TCMD\t[%s]\n", t->lexem);
			break;
		case UCMD:
			PRINTF("  UCMD\t[%s]\n", t->lexem);
			break;
		case VCMD:
			PRINTF("  VCMD\t[%s]\n", t->lexem);
			break;
		case WCMD:
			PRINTF("  WCMD\t[%s]\n", t->lexem);
			break;
		case XCMD:
			PRINTF("  XCMD\t[%s]\n", t->lexem);
			break;
		case YCMD:
			PRINTF("  YCMD\t[%s]\n", t->lexem);
			break;
		case ZCMD:
			PRINTF("  ZCMD\t[%s]\n", t->lexem);
			break;
		case END:
			PRINTF("  END\t[%s]\n", t->lexem);
			break;
		default:
			PRINTF("  UNKNOWN(%d)\t[%s]\n", t->token, t->lexem);
			break;
		}
		NEXT(t);
	}
}
