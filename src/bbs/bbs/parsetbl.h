/* THIS FILE SHOULD ONLY BE INCLUDED IN PARSE.C IT DEFINES THE TABLES */

#define OLD_CMDS 1

extern int motd(struct TOKEN *t);

#define pU		pUSER
#define pS		pSYSOP
#define pB		pBATCH
#define pR		pRESTRICT

#define pUS		(pU|pS)
#define pUSB	(pU|pS|pB)
#define pA		(pU|pS|pB|pR)

struct possible_tokens DummyOperands[] = {
	{ pA,	".",			1, 1, PERIOD, 0,			NULL, NULL },
	{ pA,	",",			1, 1, COMMA, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	"/",			1, 1, SLASH, 0,			NULL, NULL },
	{ pA,	"\\",			1, 1, BACKSLASH, 0,		NULL, NULL },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens DebugOperands[] = {
	{ pA,	"ON",			2, 2, ON, 0,				NULL, NULL },
	{ pA,	"OFF",			2, 3, OFF, 0,			NULL, NULL },
	{ pA,	"TOKENS",		3, 6, DBG_TOKENS, 0,		NULL, NULL },
	{ pA,	"FWD",			3, 3, DBG_MSGFWD, 0,		NULL, NULL },
	{ pA,	"FORWARD",		3, 7, DBG_MSGFWD, 0,		NULL, NULL },
	{ pA,	"TRANS",		3, 5, DBG_MSGTRANS, 0,	NULL, NULL },
	{ pA,	"USER",			3, 4, DBG_USER, 0,		NULL, NULL },
	{ pA,	"LIST",			3, 4, DBG_MSGLIST, 0,	NULL, NULL },
	{ pA,	"TIME",			4, 4, TIME, 0,			NULL, NULL },
	{ pA,	"NEW",			3, 3, DBG_NEW, 0,		NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens OnOffOperands[] = {
	{ pA,	"ON",			2, 2, ON, 0,				NULL, NULL },
	{ pA,	"OFF",			2, 3, OFF, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens ActOperands[] = {
	{ pS,	"HOLD",			4, 4, HOLD, 0,			NULL, NULL },
	{ pS,	"HELD",			4, 4, HOLD, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens EmailOperands[] = {
	{ pA,	"ON",			2, 2, ALL, 0,			NULL, NULL },
	{ pA,	"ALL",			2, 3, ALL, 0,			NULL, NULL },
	{ pA,	"LOCAL",		2, 5, LOCAL, 0,			NULL, NULL },
	{ pA,	"OFF",			2, 3, OFF, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens FilesysOperands[] = {
	{ pS,	"OK",			2, 2, APPROVED, 0,		NULL, NULL },
	{ pS,	"APPROVED",		2, 8, APPROVED, 0,		NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens EditOperands[] = {
	{ pS,	"PERSONAL",		3, 8, PERSONAL, 0,		NULL, NULL },
	{ pS,	"BULLETIN",		3, 8, BULLETIN, 0,		NULL, NULL },
	{ pS,	"NTS",			3, 3, NTS, 0,			NULL, NULL },
	{ pA,	"NOPROMPT",		3, 8, NOPROMPT, 0,		NULL, NULL },
	{ pS,	"SECURE",		3, 6, SECURE, 0,			NULL, NULL },
	{ pS,	"$",			1, 1, BID, 0,			NULL, NULL },
	{ pS,	"BID",			3, 3, BID, 0,			NULL, NULL },
	{ pS,	"SUBJECT",		3, 7, SUBJECT, 0,		NULL, NULL },
	{ pS,	"FORWARD",		3, 7, FORWARD, 0,		NULL, NULL },
	{ pS,	"FWD",			3, 3, FORWARD, 0,		NULL, NULL },
	{ pS,	"!",			1, 1, PASSWORD, 0,		NULL, NULL },
	{ pS,	"PASSWORD",		4, 8, PASSWORD, 0,		NULL, NULL },
	{ pS,	">",			1, 1, TO, 0,				NULL, NULL },
	{ pS,	"TO",			2, 2, TO, 0,				NULL, NULL },
	{ pS,	"<",			1, 1, FROM, 0,			NULL, NULL },
	{ pS,	"FROM",			2, 4, FROM, 0,			NULL, NULL },
	{ pS,	"@",			1, 1, AT, 0,				NULL, NULL },
	{ pS,	"AT",			2, 2, AT, 0,				NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens SignOperands[] = {
	{ pA,	"KILL",			1, 4, KILL, 0,			NULL, NULL },
	{ pA,	"WRITE",		1, 5, WRITE, 0,			NULL, NULL },
	{ pA,	"ON",			2, 2, ON, 0,				usr,	  NULL },
	{ pA,	"OFF",			2, 3, OFF, 0,			usr,	  NULL },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens WpOperands[] = {
	{ pS,	"FULL",			4, 4, FULL, 0,			NULL, NULL },
	{ pS,	"FIX",			3, 3, FIX, 0,			NULL, NULL },
	{ pS,	"KILL",			4, 4, KILL, 0,			NULL, NULL },
	{ pS,	"ALTERED",		7, 7, ALTERED, 0,		NULL, NULL },
	{ pA,	"SEARCH",		6, 6, SEARCH, 0,			NULL, NULL },
	{ pS,	"USERS",		5, 5, USER, 0,			NULL, NULL },
	{ pS,	"MAINTENANCE",	4, 11, MAINT, 0,			NULL, NULL },
	{ pS,	"GENERATE",		3, 8, GENERATE, 0,		NULL, NULL },
	{ pS,	"CUSTOM",		6, 6, CUSTOM, 0,			NULL, NULL },
	{ pS,	"WRITE",		5, 5, WRITE, 0,			NULL, NULL },
	{ pS,	"READ",			4, 4, READ, 0,			NULL, NULL },
	{ pS,	"REFRESH",		3, 7, REFRESH, 0,		NULL, NULL },
	{ pA,	"HOME",			4, 4, HOME, 0,			NULL, NULL },
	{ pA,	"LIST",			4, 4, LIST, 0,			NULL, NULL },
	{ pS,	"BBS",			3, 3, BBS, 0,			NULL, NULL },
	{ pS,	"EDIT",			4, 4, EDIT, 0,			NULL, NULL },
	{ pS,	"CHECK",		5, 5, CHECK, 0,			NULL, NULL },
	{ pS,	"TOUCH",		5, 5, TOUCH, 0,			NULL, NULL },
	{ pS,	"COMPRESS",		5, 8, COMPRESS, 0,		NULL, NULL },
	{ pA,	".",			1, 1, PERIOD, 0,			NULL, NULL },
	{ pA,	",",			1, 1, COMMA, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	"/",			1, 1, SLASH, 0,			NULL, NULL },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens MacroOperands[] = {
	{ pA,	".",			1, 1, PERIOD, 0,			NULL, NULL },
	{ pA,	",",			1, 1, COMMA, 0,			NULL, NULL },
	{ pA,	"/",			1, 1, SLASH, 0,			NULL, NULL },
	{ pA,	"\\",			1, 1, BACKSLASH, 0,		NULL, NULL },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens AddOperands[] = {
	{ pA,	"ADD",			3, 3, ADD, 0,			NULL, NULL },
	{ pA,	"DELETE",		3, 6, DELETE, 0,			NULL, NULL },
	{ pA,	"+",			1, 1, ADD, 0,			NULL, NULL },
	{ pA,	"-",			1, 1, DELETE, 0,			NULL, NULL },
	{ pA,	"NONE",			1, 4, KILL, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	",",			1, 1, COMMA, 0,			NULL, NULL },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens DelOperands[] = {
	{ pA,	"DELETE",		3, 6, DELETE, 0,			NULL, NULL },
	{ pA,	"-",			1, 1, DELETE, 0,			NULL, NULL },
	{ pA,	"NONE",			1, 4, KILL, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	",",			1, 1, COMMA, 0,			NULL, NULL },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens CheckOperands[] = {
	{ pA,	"IN",			1, 2, IN, 0,				NULL, NULL },
	{ pA,	"OUT",			1, 3, OUT, 0,			NULL, NULL },
	{ pA,	"ALL",			1, 3, ALL, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens CallOperands[] = {
	{ pA,	".",			1, 1, ADDRESS, 0,		NULL, NULL },
	{ pA,	"#",			1, 1, LOCALE, 0,			NULL, NULL },
	{ pA,	"@",			1, 1, AT, 0,				NULL, NULL },
	{ pA,	"-",			1, 1, SSID, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens EventOperands[] = {
	{ pS,	"ALL",			3, 3, ALL, 0,			NULL, NULL },
	{ pA,	"ADD",			1, 3, ADD, 0,			NULL, NULL },
	{ pS,	"COMPRESS",		8, 8, COMPRESS, 0,		NULL, NULL },
	{ pA,	"DELETE",		2, 6, DELETE, 0,			NULL, NULL },
	{ pA,	"DAYS",			2, 4, DAYS, 0,			NULL, NULL },
	{ pA,	"WEEKS",		1, 5, WEEKS, 0,			NULL, NULL },
	{ pA,	"MONTHS",		1, 6, MONTHS, 0,			NULL, NULL },
	{ pA,	"YEARS",		1, 5, YEARS, 0,			NULL, NULL },
	{ pA,	"NOPROMPT",		3, 8, NOPROMPT, 0,		NULL, NULL },
	{ pA,	"/",			1, 1, SLASH, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens KillOperands[] = {
	{ pA,	"HARD",			1, 4, HARD, 0,			NULL, NULL },
	{ pA,	"MINE",			1, 4, MINE, 0,			NULL, NULL },
	{ pS,	"NOPROMPT",		3, 8, NOPROMPT, 0,		NULL, NULL },
	{ pS,	"-",			1, 1, DASH, 0,			NULL, NULL },
	{ pA,	",",			1, 1, COMMA, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens ListOperands[] = {
	{ pA,	"ABOUT",		5, 5, ABOUT, 0,			NULL, NULL },
	{ pA,	"AT",			2, 2, AT, 0,				NULL, NULL },
	{ pA,	"ALL",			1, 3, ALL, 0,			NULL, NULL },
    { pUSB,	"ASCENDING",	6, 9, ASCEND, 0,		    NULL, NULL },
	{ pUSB,	"MINE",			1, 4, MINE, 0,			NULL, NULL },
	{ pA,	"BID",			2, 3, BID, 0,			NULL, NULL },
	{ pA,	"BULLETINS",	1, 9, BULLETIN, 0,		NULL, NULL },
	{ pUSB,	"CLUBS",		1, 5, INCLUDE, 0,		NULL, NULL },
	{ pUSB,	"CATCHUP",		5, 7, CATCHUP, 0,		NULL, NULL },
	{ pUSB,	"CHECK",		2, 5, CHECK, 0,			NULL, NULL },
	{ pUSB,	"DOWN",		    4, 4, DESCEND, 0,		NULL, NULL },
	{ pUSB,	"DESCENDING",	7, 10, DESCEND, 0,		NULL, NULL },
	{ pA,	"GROUP",		4, 5, GROUP, 0,			NULL, NULL },
	{ pA,	"HLOC",			4, 4, HLOC, 0,			NULL, NULL },
	{ pA,	"HELD",			1, 4, HELD, 0,			NULL, NULL },
	{ pUSB,	"IMMUNE",		1, 6, IMMUNE, 0,			NULL, NULL },
	{ pA,	"FILESYS",		2, 7, FILESYS, 0,		filesys, DummyOperands },
	{ pUSB,	"FORWARD",		2, 7, FORWARD, 0,		NULL, NULL },
	{ pA,	"FROM",			4, 4, FROM, 0,			NULL, NULL },
	{ pA,	"FIRST",		1, 5, FIRST, 0,			NULL, NULL },
	{ pA,	"KILLED",		1, 6, KILL, 0,			NULL, NULL },
	{ pA,	"LOCAL",		2, 5, LOCAL, 0,			NULL, NULL },
	{ pA,	"LAST",			1, 4, LAST, 0,			NULL, NULL },
	{ pUSB,	"NOPROMPT",		3, 8, NOPROMPT, 0,		NULL, NULL },
	{ pA,	"NTS",			2, 3, NTS, 0,			NULL, NULL },
	{ pA,	"NEW",			1, 3, NEW, 0,			NULL, NULL },
	{ pA,	"OLD",			1, 3, OLD, 0,			NULL, NULL },
	{ pA,	"PENDING",		4, 7, PENDING, 0,		NULL, NULL },
	{ pA,	"PERSONAL",		1, 8, PERSONAL, 0,		NULL, NULL },
	{ pA,	"SECURE",		3, 6, SECURE, 0,			NULL, NULL },
	{ pA,	"SINCE",		2, 5, SINCE, 0,			NULL, NULL },
	{ pA,	"TO",			2, 2, TO, 0,				NULL, NULL },
	{ pUSB,	"READ",			1, 4, READ, 0,			NULL, NULL },
	{ pUSB,	"UNREAD",		1, 6, UNREAD, 0,			NULL, NULL },
    { pUSB,	"UP",			2, 2, ASCEND, 0,		    NULL, NULL },
	{ pA,	"-",			1, 1, DASH, 0,			NULL, NULL },
	{ pA,	"<",			1, 1, FROM, 0,			NULL, NULL },
	{ pA,	">",			1, 1, TO, 0,				NULL, NULL },
	{ pA,	"@",			1, 1, AT, 0,				NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	"|",			1, 1, PIPE, BREAK,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens PortsOperands[] = {
	{ pA,	"TNC",			3, 3, TNC, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens DumpOperands[] = {
	{ pS,	"FULL",			2, 4, FULL, 0,			NULL, NULL },
	{ pS,	"USERS",		2, 5, USER, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens AllSendOperands[] = {
	{ pA,	"BULLETINS",	1, 9, BULLETIN, 0,		NULL, SendOperands },
	{ pA,	"DISTRIBUTION",	1, 12, DISTRIB, 0,		NULL, SendOperands },
	{ pA,	"INTERNET",		1, 8, MAIL, 0,			NULL, SendOperands },
	{ pA,	"PERSONAL",		1, 8, PERSONAL, 0,		NULL, SendOperands },
	{ pA,	"SECURE",		1, 8, SECURE, 0,			NULL, SendOperands },
	{ pA,	"NTS",			2, 3, NTS, 0,			NULL, SendOperands },
	{ pA,	"FILESYS",		2, 7, FILESYS, 0,		filesys, DummyOperands },
	{ pA,	"T",			1, 1, NTS, 0,			NULL, SendOperands },
	{ pA,	"<",			1, 1, FROM, 0,			NULL, NULL },
	{ pA,	"FROM",			4, 4, FROM, 0,			NULL, NULL },
	{ pA,	">",			1, 1, TO, 0,				NULL, NULL },
	{ pA,	"@",			1, 1, AT, 0,				NULL, NULL },
	{ pA,	"AT",			2, 2, AT, 0,				NULL, NULL },
#if 0
	{ pA,	"!",			1, 1, PASSWORD, 0,		NULL, NULL },
#endif
	{ pA,	"$",			1, 1, BID, 0,			NULL, NULL },
	{ pA,	"BID",			3, 3, BID, 0,			NULL, NULL },
	{ pA,	".",			1, 1, ADDRESS, 0,		NULL, NULL },
	{ pA,	"#",			1, 1, LOCALE, 0,			NULL, NULL },
	{ pA,	"-",			1, 1, SSID, 0,			NULL, NULL },
	{ pA,	"_",			1, 1, WORD, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens InclOperands[] = {
	{ pA,	"<",			1, 1, FROM, 0,			NULL, NULL },
	{ pA,	">",			1, 1, TO, 0,				NULL, NULL },
	{ pA,	"@",			1, 1, AT, 0,				NULL, NULL },
	{ pA,	"DELETE",		6, 6, DELETE, 0,			NULL, NULL },
	{ pA,	"ADD",			3, 3, ADD, 0,			NULL, NULL },
	{ pA,	"CLEAR",		5, 5, KILL, 0,			NULL, NULL },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens SendOperands[] = {
	{ pA,	"<",			1, 1, FROM, 0,			NULL, NULL },
	{ pA,	">",			1, 1, TO, 0,				NULL, NULL },
	{ pA,	"@",			1, 1, AT, 0,				NULL, NULL },
	{ pA,	"!",			1, 1, PASSWORD, 0,		NULL, NULL },
	{ pA,	"$",			1, 1, BID, 0,			NULL, NULL },
	{ pA,	".",			1, 1, ADDRESS, 0,		NULL, NULL },
	{ pA,	"#",			1, 1, LOCALE, 0,			NULL, NULL },
	{ pA,	"-",			1, 1, SSID, 0,			NULL, NULL },
	{ pA,	"_",			1, 1, WORD, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens WxOperands[] = {
	{ pA,	"INDOOR",		1, 6, INDOOR, 0,			NULL, NULL },
	{ pA,	"BAROMETER",	1, 9, BAROMETER, 0,		NULL, NULL },
	{ pA,	"HUMIDITY",		1, 8, HUMIDITY, 0,		NULL, NULL },
	{ pA,	"RAIN",			1, 4, RAIN, 0,			NULL, NULL },
	{ pA,	"GRAPH",		1, 5, GRAPH, 0,			NULL, NULL },
	{ pA,	"DATA",			1, 4, DATA, 0,			NULL, NULL },
	{ pA,	"TEMPERATURE",	1, 11,TEMPERATURE, 0,	NULL, NULL },
	{ pA,	"YESTERDAY",	1, 9, YESTERDAY, 0,		NULL, NULL },
	{ pA,	"WIND",			1, 4, WIND, 0,			NULL, NULL },
	{ pA,	"SET",			1, 4, SET, 0,			NULL, NULL },
	{ pS,	"LOG",			1, 3, LOG, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens InitiateOperands[] = {
	{ pA,	"WP",			1, 2, WP, 0,				NULL, NULL },
	{ pA,	"FILESYS",		2, 7, FILESYS, 0,		NULL, NULL },
	{ pA,	"CALLBK",		1, 6, CALLBK, 0,			NULL, NULL },
	{ pA,	"EVENT",		1, 5, EVENT, 0,			NULL, NULL },
	{ pA,	"FORWARDING",	2, 10, FORWARD, 0,		NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens ReadOperands[] = {
	{ pA,	"FILESYS",		2, 7, FILESYS, 0,		filesys, DummyOperands },
	{ pA,	"HEADER",		1, 6, HEADER, 0,			NULL, NULL },
	{ pA,	"MINE",			1, 4, MINE, 0,			NULL, NULL },
	{ pA,	"MOTD",			2, 4, MOTD, 0,			NULL, NULL },
	{ pA,	"NOPROMPT",		3, 8, NOPROMPT, 0,		NULL, NULL },
	{ pA,	"WHY",			3, 3, WHY, 0,			NULL, NULL },
	{ pA,	"WHOM",			1, 4, WHO, 0,			NULL, NULL },
	{ pA,	"TOUCH",		1, 5, TOUCH, 0,			NULL, NULL },
	{ pA,	",",			1, 1, COMMA, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens LoadOperands[] = {
	{ pA,	"TNC144",		4, 6, TNC144, 0,			NULL, NULL },
	{ pA,	"TNC220",		4, 6, TNC220, 0,			NULL, NULL },
	{ pA,	"TNC440",		4, 6, TNC440, 0,			NULL, NULL },
	{ pA,	"PHONE",		1, 5, PHONE, 0,			NULL, NULL },
	{ pA,	"CONSOLE",		1, 7, CONSOLE, 0,		NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens NameOperands[] = {
	{ pA,	"NAME",			1, 4, WHO, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens UserOperands[] = {
	{ pS,	"AGE",			3, 3, AGE, 0,			NULL, NULL },
	{ pS,	"BBS",			2, 3, BBS, 0,			NULL, NULL },
	{ pS,	"CHECK",		2, 5, CHECK, 0,			NULL, NULL },
	{ pS,	"COMPUTER",		2, 8, COMPUTER, 0,		NULL, NULL },
	{ pS,	"CUSTOM",		2, 6, CUSTOM, 0,			NULL, NULL },
	{ pS,	"HDX",			3, 3, HDX, 0,			NULL, NULL },
	{ pS,	"EMAIL",		2, 5, EMAIL, 0,			NULL, NULL },
	{ pS,	"EXPIRED",		2, 7, EXPIRED, 0,		NULL, NULL },
	{ pS,	"FIRST",		3, 5, FNAME, 0,			NULL, NULL },
	{ pS,	"FIX",			3, 3, FIX, 0,			NULL, NULL },
	{ pS,	"FNAME",		2, 5, FNAME, 0,			NULL, NULL },
	{ pS,	"HOME",			2, 4, HOME, 0,			NULL, NULL },
	{ pS,	"IMMUNE",		2, 6, IMMUNE, 0,			NULL, NULL },
	{ pS,	"KILL",			2, 4, KILL, 0,			NULL, NULL },
	{ pS,	"LNAME",		2, 5, LNAME, 0,			NULL, NULL },
	{ pS,	"LAST",			2, 4, LNAME, 0,			NULL, NULL },
	{ pS,	"NEW",			2, 3, NEW, 0,			NULL, NULL },
	{ pS,	"NONHAM",		2, 6, NONHAM, 0,			NULL, NULL },
	{ pS,	"QTH",			1, 3, QTH, 0,			NULL, NULL },
	{ pS,	"REGEXP",		6, 6, REGEXP, 0,			NULL, NULL },
	{ pS,	"RESTRICTED",	2, 10, RESTRICTED, 0,	NULL, NULL },
	{ pS,	"RIG",			2, 3, RIG, 0,			NULL, NULL },
	{ pS,	"SOFTWARE",		2, 8, SOFTWARE, 0,		NULL, NULL },
	{ pS,	"STATUS",		2, 6, STATUS, 0,			NULL, NULL },
	{ pS,	"SUSPECT",		2, 7, SUSPECT, 0,		NULL, NULL },
	{ pS,	"SYSOP",		2, 5, SYSOP, 0,			NULL, NULL },
	{ pS,	"TNC",			1, 3, TNC, 0,			NULL, NULL },
	{ pS,	"ZIP",			1, 3, ZIP, 0,			NULL, NULL },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens SetOperands[] = {
	{ pS,	"ALL",			2, 3, ALL, 0,			NULL, NULL },
	{ pS,	"APPROVED",		2, 8, APPROVED, 0,		NULL, NULL },
	{ pS,	"DESCENDING",	7, 10, DESCEND, 0,		NULL, NULL },
    { pS,	"ASCENDING",	6, 9, ASCEND, 0,		    NULL, NULL },
	{ pS,	"FNAME",		1, 5, FNAME, 0,			NULL, NULL },
	{ pS,	"FIRSTNAME",	6, 9, FNAME, 0,			NULL, NULL },
	{ pS,	"FIRST",		2, 5, FNAME, 0,			NULL, NameOperands },
	{ pS,	"CLUB",			2, 4, INCLUDE, 0,		NULL, AddOperands },
	{ pS,	"COMPUTER",		3, 8, COMPUTER, 0,		NULL, NULL },
	{ pS,	"CONSOLE",		3, 7, CONSOLE, 0,		NULL, NULL },
	{ pS,	"CUSTOM",		2, 6, CUSTOM, 0,			NULL, NULL },
	{ pS,	"HDX",			4, 4, HDX, 0,			NULL, NULL },
	{ pS,	"EMAIL",		2, 5, EMAIL, 0,			NULL, OnOffOperands },
	{ pS,	"EQUIP",		2, 5, EQUIP, 0,			NULL, NULL },
	{ pS,	"FAST",			2, 4, FAST, 0,			NULL, NULL },
	{ pS,	"FORWARD",		2, 7, FORWARD, 0,		NULL, NULL },
	{ pS,	"FREQUENCY",	2, 9, FREQ, 0,			NULL, NULL },
	{ pS,	"HELP",			2, 4, HELP, 0,			NULL, NULL },
	{ pS,	"HOME",			2, 4, HOME, 0,			NULL, NULL },
	{ pS,	"IMMUNE",		2, 6, IMMUNE, 0,			NULL, NULL },
	{ pS,	"LINES",		2, 5, LINES, 0,			NULL, NULL },
	{ pS,	"LNAME",		2, 5, LNAME, 0,			NULL, NULL },
	{ pS,	"LASTNAME",		5, 8, LNAME, 0,			NULL, NULL },
	{ pS,	"LAST",			2, 4, LNAME, 0,			NULL, NameOperands },
	{ pS,	"LOGGING",		2, 7, LOG, 0,			NULL, NULL },
	{ pS,	"MACRO",		2, 5, MACRO, 0,			NULL, NULL },
	{ pS,	"MOTD",			2, 5, MOTD, 0,			NULL, NULL },
	{ pS,	"NEWLINE",		2, 7, NEWLINE, 0,		NULL, NULL },
	{ pS,	"NONHAM",		4, 6, NONHAM, 0,			NULL, NULL },
	{ pS,	"PASSWORD",		2, 8, PASSWORD, 0,		NULL, AddOperands },
	{ pS,	"PHONE",		2, 5, PHONE, 0,			NULL, NULL },
	{ pS,	"QTH",			1, 3, QTH, 0,			NULL, NULL },
	{ pS,	"REGEXP",		6, 6, REGEXP, 0,			NULL, NULL },
	{ pS,	"RESTRICTED",	2, 10, RESTRICTED, 0,	NULL, NULL },
	{ pS,	"RIG",			1, 3, RIG, 0,			NULL, NULL },
	{ pS,	"SKIP",			2, 4, EXCLUDE, 0,		NULL, AddOperands },
	{ pS,	"SLOW",			2, 4, SLOW, 0,			NULL, NULL },
	{ pS,	"SOFTWARE",		2, 8, SOFTWARE, 0,		NULL, NULL },
	{ pS,	"SUSPECT",		2, 7, SUSPECT, 0,		NULL, NULL },
	{ pS,	"SYSOP",		2, 5, SYSOP, 0,			NULL, NULL },
	{ pS,	"TNC144",		4, 6, TNC144, 0,			NULL, NULL },
	{ pS,	"TNC220",		4, 6, TNC220, 0,			NULL, NULL },
	{ pS,	"TNC440",		4, 6, TNC440, 0,			NULL, NULL },
	{ pS,	"TNC",			1, 3, TNC, 0,			NULL, NULL },
	{ pS,	"UUCP",			1, 4, EMAIL, 0,			NULL, OnOffOperands },
	{ pS,	"ZIP",			1, 3, ZIP, 0,			NULL, NULL },
	{ pS,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pS,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens WriteOperands[] = {
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens DistribOperands[] = {
	{ pA,	"LIST",			1, 4, LIST, 0,			NULL, WriteOperands },
	{ pA,	"KILL",			1, 4, KILL, 0,			NULL, WriteOperands },
	{ pA,	"READ",			1, 4, READ, 0,			NULL, WriteOperands },
	{ pA,	"WRITE",		1, 5, WRITE, 0,			NULL, WriteOperands },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, Opcodes },
	{ pA,	 NULL,			0, 0, 0, 0,			NULL, NULL }};

struct possible_tokens HelpOperands[] = {
	{ pA,	"ACTIVATE",		3, 8, ACTIVATE,	958,NULL,NULL },
	{ pA,	"ADDRESS",		4, 7, ADDRESS,	940,NULL,NULL },
	{ pA,	"ADD",			2, 3, ADD,		941,NULL,NULL },
	{ pA,	"ADVENTURE",	3, 9, ADVENTURE,	1005,NULL,NULL },
	{ pA,	"ALL",			2, 3, ALL,		942,NULL,NULL },
	{ pA,   "AXRELAY",	3, 7, AXRELAY,		1004,NULL,NULL },
	{ pA,	"A",			1, 0, ACMD,		901,NULL,NULL },

	{ pA,	"BAROMETER",	2, 9, BAROMETER,943,NULL,NULL },
	{ pA,	"BULLETINS",	2, 9, BULLETIN,	942,NULL,NULL },
	{ pA,	"BYE",			2, 3, BYE,		944,NULL,NULL },
	{ pA,	"B",			1, 0, BCMD,		902,NULL,NULL },

	{ pA,	"CALLBK",		2, 6, CALLBK,	953,NULL,NULL },
	{ pA,	"CD",			2, 2, CD,		945,NULL,NULL },
	{ pA,	"CHECK",		2, 5, CHECK,	946,NULL,NULL },
	{ pA,	"CLUBS",		2, 5, INCLUDE,	947,NULL,NULL },
	{ pA,	"COMPUTER",		4, 8, COMPUTER, 948,NULL,NULL },
	{ pA,	"COMMANDS",		4, 8, COMMANDS,	  0,NULL,NULL },
	{ pA,	"CMDS",			2, 8, COMMANDS,	  0,NULL,NULL },
	{ pA,	"COPY",			3, 4, COPY,		949,NULL,NULL },
	{ pA,	"C",			1, 0, CCMD,		903,NULL,NULL },

	{ pA,	"DAYS",			2, 4, DAYS,		952,NULL,NULL },
	{ pA,	"DELETE",		2, 6, DELETE,	941,NULL,NULL },
	{ pA,	"DIR",			2, 3, DIRECTORY,950,NULL,NULL },
	{ pA,	"DISTRIBUTION", 2, 12, DISTRIB, 939,NULL,NULL },
	{ pA,	"D",			1, 0, DCMD,		904,NULL,NULL },

	{ pA,	"EQUIPMENT",	2, 9, EQUIP,	948,NULL,NULL },
	{ pA,	"EVENT",		2, 5, EVENT,	952,NULL,NULL },
	{ pA,	"EDIT",			2, 4, EDIT,		951,NULL,NULL },
	{ pA,	"EMAIL",		2, 5, EMAIL,	981,NULL,NULL },
	{ pA,	"EXIT",			2, 4, BYE,		944,NULL,NULL },
	{ pA,	"E",			1, 0, ECMD,		905,NULL,NULL },

	{ pA,	"FILESYS",		3, 7, FILESYS,	954,NULL,NULL },
	{ pA,	"FIRST",		3, 5, FIRST,	942,NULL,NULL },
	{ pA,	"FNAME",		2, 5, FNAME,	955,NULL,NULL },
	{ pA,	"FIRSTNAME",	2, 9, FNAME,	955,NULL,NULL },
	{ pA,	"FORWARD",		2, 10, FORWARD, 956,NULL,NULL },
	{ pA,	"FREQUENCY",	2, 9, FREQ,		990,NULL,NULL },
	{ pA,	"FULL",			2, 4, FULL,		  0,NULL,NULL }, /*?*/
	{ pA,	"F",			1, 0, FCMD,		906,NULL,NULL },

	{ pA,	"GRAPH",		2, 5, GRAPH,	943,NULL,NULL },
	{ pA,	"GAMES",	4, 5, GAMES,		1006,NULL,NULL },
	{ pA,	"G",			1, 0, GCMD,		907,NULL,NULL },

	{ pA,	"HEADER",		3, 6, HEADER,	957,NULL,NULL },
	{ pA,	"HELD",			4, 4, HELD,		942,NULL,NULL },
	{ pA,	"HISTORY",		2, 7, HISTORY,	937,NULL,NULL },
	{ pA,	"HOLD",			3, 4, HOLD,		958,NULL,NULL },
	{ pA,	"HOMEBBS",		3, 7, HOME,		955,NULL,NULL },
	{ pA,	"HELP",			4, 4, HELP,		900,NULL,NULL },
	{ pA,	"HUMIDITY",		2, 8, HUMIDITY, 943,NULL,NULL },
	{ pA,	"H",			1, 0, HCMD,		908,NULL,NULL },

	{ pA,	"INITIATE",		3, 8, INITIATE, 960,NULL,NULL },
	{ pA,	"INFO",			3, 4, INFO,		961,NULL,NULL },
	{ pA,	"INCLUDE",		3, 7, INCLUDE,	947,NULL,NULL },
	{ pA,	"IN",			2, 2, IN,		946,NULL,NULL },
	{ pA,	"IMMUNE",		2, 6, IMMUNE,	909,NULL,NULL },
	{ pA,	"I",			1, 0, ICMD,		909,NULL,NULL },

	{ pA,	"J",			1, 0, JCMD,		910,NULL,NULL },

	{ pA,	"KM",			2, 2, KILL,		962,NULL,NULL },
	{ pA,	"KH",			2, 2, KILL,		962,NULL,NULL },
	{ pA,	"KILL",			2, 6, KILL,		962,NULL,NULL },
	{ pA,	"KEYWORDS",		2, 8, KCMD,		985,NULL,NULL },
	{ pA,	"K",			1, 0, KCMD,		911,NULL,NULL },

	{ pA,	"LAST",			3, 4, LAST,		942,NULL,NULL },
	{ pA,	"LISTING",		5, 7, LIST,	987,NULL,NULL },
	{ pA,	"LINES",		3, 5, LINES,	963,NULL,NULL },
	{ pA,	"LNAME",		3, 5, LNAME,	964,NULL,NULL },
	{ pA,	"LASTNAME",		3, 8, LNAME,	964,NULL,NULL },
	{ pA,	"LOAD",			4, 4, LOAD,		938,NULL,NULL },
	{ pA,	"LOOKUP",		3, 6, LOOKUP,	953,NULL,NULL },
	{ pA,	"LOCALE",		6, 6, LOCALE,	940,NULL,NULL },
	{ pA,	"LOCAL",		3, 5, LOCAL,	942,NULL,NULL },
	{ pA,	"LA",			2, 2, LIST,		942,NULL,NULL },
	{ pA,	"LB",			2, 2, LIST,		942,NULL,NULL },
	{ pA,	"LF",			2, 2, LIST,		942,NULL,NULL },
	{ pA,	"LH",			2, 2, LIST,		942,NULL,NULL },
	{ pA,	"LS",			2, 2, LS,		950,NULL,NULL },
	{ pA,	"LL",			2, 2, LIST,		942,NULL,NULL },
	{ pA,	"LP",			2, 2, LIST,		942,NULL,NULL },
	{ pA,	"LR",			2, 2, LIST,		942,NULL,NULL },
	{ pA,	"LO",			2, 2, LIST,		942,NULL,NULL },
	{ pA,	"LK",			2, 2, LIST,		942,NULL,NULL },
	{ pA,	"LM",			2, 2, LIST,		942,NULL,NULL },
	{ pA,	"LN",			2, 2, LIST,		942,NULL,NULL },
	{ pA,	"LT",			2, 2, LIST,		942,NULL,NULL },
	{ pA,	"LC",			2, 2, LIST,		942,NULL,NULL },
	{ pA,	"LU",			2, 2, LIST,		942,NULL,NULL },
	{ pA,	"LIST",			2, 4, LIST,		942,NULL,NULL },
	{ pA,	"L",			1, 0, LCMD,		912,NULL,NULL },

	{ pA,	"MACRO",		2, 5, MACRO,	965,NULL,NULL },
	{ pA,	"ME",			2, 2, ME,		967,NULL,NULL },
	{ pA,	"MINE",			2, 4, MINE,		968,NULL,NULL },
	{ pA,	"MONTHS",		3, 6, MONTHS,	952,NULL,NULL },
	{ pA,	"MOTD",			3, 4, MOTD,		969,NULL,NULL },
	{ pA,	"M",			1, 0, MCMD,		913,NULL,NULL },

	{ pA,	"NEW",			2, 3, NEW,		942,NULL,NULL },
	{ pA,	"NOPROMPT",		3, 8, NOPROMPT, 963,NULL,NULL },
	{ pA,	"NTS",			2, 3, NTS,		970,NULL,NULL },
	{ pA,	"N",			1, 0, NCMD,		914,NULL,NULL },

	{ pA,	"OLD",			2, 3, OLD,		942,NULL,NULL },
	{ pA,	"OUT",			2, 3, OUT,		946,NULL,NULL },
	{ pA,	"O",			1, 0, OCMD,		915,NULL,NULL },

	{ pA,	"PASSWORD",		2, 8, PASSWORD, 971,NULL,NULL },
	{ pA,	"PENDING",		3, 7, PENDING,	942,NULL,NULL },
	{ pA,	"PERSONAL",		3, 8, PERSONAL, 942,NULL,NULL },
	{ pA,	"PHONE",		2, 5, PHONE,	971,NULL,NULL },
	{ pA,	"PORTS",		2, 5, PORTS,	972,NULL,NULL },
	{ pA,	"PROCESS",		2, 7, PROCESS,	962,NULL,NULL },
	{ pA,	"P",			1, 0, PCMD,		916,NULL,NULL },

	{ pA,	"QTH",			2, 3, QTH,		955,NULL,NULL },
	{ pA,	"QUIT",			2, 4, BYE,		944,NULL,NULL },
	{ pA,	"Q",			1, 0, QCMD,		917,NULL,NULL },

	{ pA,	"RAIN",			2, 4, RAIN,		943,NULL,NULL },
	{ pA,	"RELEASE",		3, 7, RELEASE,	958,NULL,NULL },
	{ pA,	"RIG",			2, 3, RIG,		948,NULL,NULL },
	{ pA,	"REPLY",		3, 5, REPLY,	973,NULL,NULL },
	{ pA,	"READ",			3, 4, READ,		974,NULL,NULL },
	{ pA,	"RM",			2, 2, READ,		974,NULL,NULL },
	{ pA,	"RH",			2, 2, READ,		974,NULL,NULL },
	{ pA,	"RW",			2, 2, READ,		974,NULL,NULL },
	{ pA,	"RESERVED",		3, 8, RCMD,		985,NULL,NULL },
	{ pA,	"REMOTE",		3, 6, REMOTE,	998,NULL,NULL },
	{ pA,	"REFRESH",		3, 7, REFRESH,	918,NULL,NULL },
	{ pA,	"R",			1, 0, RCMD,		918,NULL,NULL },

	{ pA,	"SSID",			3, 4, SSID,		975,NULL,NULL },
	{ pA,	"SB",			2, 2, SEND,		976,NULL,NULL },
	{ pA,	"SD",			2, 2, SEND,		976,NULL,NULL },
	{ pA,	"SEARCH",		3, 6, SEARCH,	953,NULL,NULL },
	{ pA,	"SECURE",		3, 6, SECURE,	976,NULL,NULL },
	{ pA,	"SET",			3, 3, SET,		988,NULL,NULL },
	{ pA,	"SHELL",		3, 5, SHELL,	977,NULL,NULL },
	{ pA,	"SIGNATURE",	3, 9, SIGNATURE,993,NULL,NULL },
	{ pA,	"SI",			2, 2, SEND,		976,NULL,NULL },
	{ pA,	"SHOW",			3, 4, SHOW,		989,NULL,NULL },
	{ pS,	"SKIP",			2, 4, EXCLUDE,	978,NULL,NULL },
	{ pA,	"SOFTWARE",		2, 8, SOFTWARE, 948,NULL,NULL },
	{ pA,	"SPAWN",		3, 5, SPAWN,	979,NULL,NULL },
	{ pA,	"SP",			2, 2, SEND,		976,NULL,NULL },
	{ pA,	"SR",			2, 2, SEND,		973,NULL,NULL },
	{ pA,	"ST",			2, 2, SEND,		976,NULL,NULL },
	{ pA,	"SS",			2, 2, SEND,		976,NULL,NULL },
	{ pA,	"SEND",			2, 4, SEND,		976,NULL,NULL },
	{ pA,	"SYSTEM",		4, 6, BREAK,	  0,NULL,NULL },
	{ pA,	"SYSOP",		4, 5, BREAK,	984,NULL,NULL },
	{ pA,	"S",			1, 0, SCMD,		919,NULL,NULL },

	{ pA,	"TEMPERATURE",	2, 11, TEMPERATURE, 943,NULL,NULL },
	{ pA,	"TNC144",		4, 6, TNC144,	979,NULL,NULL },
	{ pA,	"TNC220",		4, 6, TNC220,	979,NULL,NULL },
	{ pA,	"TNC440",		4, 6, TNC440,	979,NULL,NULL },
	{ pA,	"TNC",			2, 3, TNC,		948,NULL,NULL },
	{ pA,	"T",			1, 0, TCMD,		920,NULL,NULL },

	{ pA,	"USERS",		2, 5, USER,		980,NULL,NULL },
	{ pA,	"UUCP",			2, 4, EMAIL,	981,NULL,NULL },
	{ pA,	"U",			1, 0, UCMD,		921,NULL,NULL },

	{ pA,	"VACATION",		2, 8, VACATION, 102,NULL,NULL },
	{ pA,	"V",			1, 0, VCMD,		922,NULL,NULL },

	{ pA,	"WEEKS",		2, 5, WEEKS,	952,NULL,NULL },
	{ pA,	"WHEREIS",		3, 7, WHEREIS,	982,NULL,NULL },
	{ pA,	"WHO",			3, 4, WHO,		967,NULL,NULL },
	{ pA,	"WIND",			2, 4, WIND,		943,NULL,NULL },
	{ pA,	"WP",			2, 2, WP,		982,NULL,NULL },
	{ pA,	"WRITE",		2, 5, WRITE,	983,NULL,NULL },
	{ pA,	"WX",			2, 2, WX,		943,NULL,NULL },
	{ pA,	"W",			1, 0, WCMD,		923,NULL,NULL },

	{ pA,	"X",			1, 0, XCMD,		924,NULL,NULL },

	{ pA,	"Y",			1, 0, YCMD,		925,NULL,NULL },

	{ pA,	"ZIP",			2, 3, ZIP,		955,NULL,NULL },
	{ pA,	"Z",			1, 0, ZCMD,		926,NULL,NULL },

	{ pA,	"<",			1, 1, FROM,		927,NULL,NULL },
	{ pA,	">",			1, 1, TO,		927,NULL,NULL },
	{ pA,	"@",			1, 1, AT,		940,NULL,NULL },
	{ pA,	"!",			1, 1, PASSWORD, 927,NULL,NULL },
	{ pA,	"$",			1, 1, BID,		927,NULL,NULL },
	{ pA,	"#",			1, 1, LOCALE,	940,NULL,NULL },
	{ pA,	".",			1, 1, ADDRESS,	940,NULL,NULL },
	{ pA,	"-",			1, 1, DELETE,	941,NULL,NULL },
	{ pA,	"+",			1, 1, ADD,		941,NULL,NULL },
	{ pA,	"*",			1, 1, ALL,		900,NULL,NULL },
	{ pA,	"?",			1, 1, HELP,		900,NULL,NULL },

	{ pA,	 NULL,			0, 0, 0,		0,NULL,NULL }};

#define exitbbs	(int (*)())exit_bbs

struct possible_tokens Opcodes[] = {
	{ pA,	"TEST",			4, 4, ME, 0,			system_open, DummyOperands },
	{ pS,	"ACTIVATE",		3, 8, ACTIVATE, 0,	msg,		ActOperands },
	{ pA,   "ADVENTURE",	3, 9,  ADVENTURE, 0,	adventure,	DummyOperands },
	{ pUS,	"BASE",			2, 4, BASE, 0,		usr,		DummyOperands },
	{ pUS,	"BYE",			1, 3, BYE, 0,		maint,		DummyOperands },
	{ pA,	"CD",			2, 2, CD, 0,			filesys,	DummyOperands },
	{ pUS,	"CHAT",			3, 4, CHAT, 0,		chat,		DummyOperands },
	{ pUS,	"CHECK",		2, 5, CHECK, 0,		msg,		CheckOperands },
	{ pUS,	"CHK",			2, 3, CHECK, 0,		msg,		CheckOperands },
	{ pUS,	"CHECKOUT",		6, 8, CHECK, OUT,		msg,		CheckOperands },
	{ pUS,	"CHKOUT",		4, 6, CHECK, OUT,		msg,		CheckOperands },
	{ pUS,	"CHECKIN",		6, 7, CHECK, IN,		msg,		CheckOperands },
	{ pUS,	"CHKIN",		4, 5, CHECK, IN,		msg,		CheckOperands },
	{ pUSB,	"CLEAR",		3, 5, CLEAR, 0,		usr,		SetOperands },
	{ pUSB,	"CLR",			3, 3, CLEAR, 0,		usr,		SetOperands },
	{ pUSB,	"CLUBS",		2, 5, INCLUDE, 0,	usr,		InclOperands },
	{ pS,	"COMPRESS",		8, 8, COMPRESS, 0,	maint,		DummyOperands },
	{ pUSB,	"COMPUTER",		3, 8, COMPUTER, 0,	usr,		DelOperands },
	{ pUSB,	"COPY",			2, 4, COPY, 0,		msg,		CallOperands },
	{ pA,	"DEBUG",		5, 5, DEBUG, 0,		maint,		DebugOperands },
	{ pA,	"DIR",			3, 3, DIRECTORY, 0,	filesys,	DummyOperands },
	{ pUSB,	"DISTRIBUTION",	4, 12, DISTRIB, 0,	distrib_t,	DistribOperands },
	{ pUSB,	"HDX",			2, 4, HDX, 0,		usr,	 	OnOffOperands },
	{ pUSB,	"EMAIL",		2, 5, EMAIL, 0,		usr,		EmailOperands },
	{ pA,	"ECHO",			2, 4, ECHOCMD, 0,	cmd,		DummyOperands },
	{ pUSB,	"EQUIPMENT",	2, 9, EQUIP, 0,		usr,		DummyOperands },
	{ pUSB,	"EVENT",		2, 5, EVENT, 0,		event_t,	EventOperands },
	{ pS,	"EDIT",			2, 4, EDIT, 0,		msg,		EditOperands },
	{ pUSB,	"EXCLUDE",		3, 7, EXCLUDE, 0,	usr,		InclOperands },
	{ pUSB,	"EXIT",			2, 4, BYE, HARD,	maint,		DummyOperands },
	{ pS,	"FILESYS",		2, 7, FILESYS, 0,	filesys,	FilesysOperands },
	{ pUSB,	"FNAME",		2, 5, FNAME, 0,		usr,		DummyOperands },
	{ pUSB,	"FIRSTNAME",	6, 9, FNAME, 0,		usr,		DummyOperands },
	{ pUSB,	"FIRST",		2, 5, FNAME, 0,		usr,	 	NameOperands },
/*	{ pS,	"FORWARD",		2, 7, FORWARD, 0,	msg,		CallOperands }, */
	{ pUSB,	"FREQUENCY",	2, 9, FREQ, 0,		usr,		DummyOperands },
	{ pUS,	"F",			1, 1, REVFWD, 0,		msg,		DummyOperands },
	{ pUSB,	"GROUP",		1, 5, GROUP, 0,		msg,	 	OnOffOperands },
	{ pUSB,	"HISTORY",		2, 7, HISTORY, 0,	history,	DummyOperands },
	{ pUSB,	"HOLD",			3, 4, HOLD, 0,		msg,		DummyOperands },
	{ pUSB,	"HOMEBBS",		2, 7, HOME, 0,		usr,		DummyOperands },
	{ pA,	"HELP",			1, 4, HELP, 0,		help,		HelpOperands },
	{ pS,	"INITIATE",		3, 8, INITIATE, 0,	initiate,	InitiateOperands },
	{ pUSB,	"INCLUDE",		3, 7, INCLUDE, 0,	usr,		InclOperands },
	{ pS,	"IMMUNE",		3, 6, IMMUNE, 0,		msg,		DummyOperands },
	{ pA,	"INFO",			1, 4, INFO, 0,		information,DummyOperands },
	{ pUS,	"KH",			2, 2, KILL, HARD,		msg,		KillOperands },
	{ pUSB,	"KM",			2, 2, KILL, MINE,		msg,		KillOperands },
	{ pS,	"KN",			2, 2, KILL, NOPROMPT,	msg,		KillOperands },
	{ pUSB,	"KILL",			1, 6, KILL, 0,		msg,		KillOperands },
	{ pA,	"LOAD",			4, 4, LOAD, 0,		load_t,		LoadOperands },
	{ pUSB,	"LINES",		3, 5, LINES, 0,		usr,		DummyOperands },
	{ pUSB,	"LNAME",		3, 5, LNAME, 0,		usr,		DummyOperands },
	{ pUSB,	"LASTNAME",		5, 8, LNAME, 0,		usr,		DummyOperands },
	{ pUSB,	"LAST",			3, 4, LNAME, 0,		usr,	 	NameOperands },
	{ pUS,	"LOGOUT",		4, 6, BYE, 0,		maint,	DummyOperands },
	{ pUS,	"LOGOFF",		4, 6, BYE, 0,		maint,	DummyOperands },
	{ pA,	"LOOKUP",		3, 6, LOOKUP, 0,		callbk,		DummyOperands },
	{ pA,	"LA",			2, 2, LIST, ALL,		msg,		ListOperands },
	{ pA,	"LB",			2, 2, LIST, BULLETIN,	msg,		ListOperands },
	{ pA,	"LF",			2, 2, LIST, FIRST,		msg,		ListOperands },
	{ pA,	"LG",			2, 2, LIST, GROUP,		msg,		ListOperands },
	{ pA,	"LS",			2, 2, DIRECTORY, 0,	filesys,	DummyOperands },
	{ pUSB,	"LH",			2, 2, LIST, HELD,		msg,		ListOperands },
	{ pA,	"LL",			2, 2, LIST, LAST,		msg,		ListOperands },
	{ pA,	"LP",			2, 2, LIST, PERSONAL,	msg,		ListOperands },
	{ pUSB,	"LR",			2, 2, LIST, READ,		msg,		ListOperands },
	{ pA,	"LO",			2, 2, LIST, OLD,		msg,		ListOperands },
	{ pUSB,	"LK",			2, 2, LIST, KILL,		msg,		ListOperands },
	{ pUSB,	"LM",			2, 2, LIST, MINE,		msg,		ListOperands },
	{ pUSB,	"LN",			2, 2, LIST, NEW,		msg,		ListOperands },
	{ pUSB,	"LC",			2, 2, LIST, INCLUDE,	msg,		ListOperands },
	{ pUSB,	"LT",			2, 2, LIST, NTS,		msg,		ListOperands },
	{ pUSB,	"LU",			2, 2, LIST, UNREAD,		msg,		ListOperands },
	{ pA,	"LIST",			1, 4, LIST, 0,		msg,		ListOperands },
	{ pUSB,	"MACRO",		2, 5, MACRO, 0,		usr,		MacroOperands },
	{ pA,	"ME",			2, 2, ME, 0,			usr,		DummyOperands },
	{ pA,	"MOTD",			2, 4, MOTD, 0,		motd,		DummyOperands },
	{ pUSB,	"NEWLINE",		2, 7, NEWLINE, 0,	usr,	 	OnOffOperands },
	{ pUSB,	"PHONE",		2, 5, PHONE, 0,		usr,		DelOperands },
	{ pA,	"PORTS",		2, 5, PORTS, 0,		ports,		DummyOperands },
	{ pUSB,	"QTH",			2, 3, QTH, 0,		usr,		DummyOperands },
	{ pUS,	"QUIT",			1, 4, BYE, 0,		maint,	DummyOperands },
	{ pS,	"ROUTE",		5, 5, ROUTE, 0,		msg,		DummyOperands },
	{ pUSB,	"REGEXP",		6, 6, REGEXP, 0,		usr,	 	OnOffOperands },
	{ pS,	"REFRESH",		3, 7, REFRESH, 0,	msg,		DummyOperands },
	{ pS,	"RELEASE",		3, 7, RELEASE, 0,	msg,		DummyOperands },
	{ pUSB,	"RIG",			2, 3, RIG, 0,		usr,		DelOperands },
	{ pUS,	"REPLY",		3, 5, REPLY, 0,		msg,		DummyOperands },
	{ pA,	"READ",			1, 4, READ, 0,		msg,		ReadOperands },
	{ pA,	"CAT",			3, 3, READ, 0,		msg,		ReadOperands },
	{ pA,	"TYPE",			4, 4, READ, 0,		msg,		ReadOperands },
	{ pS,	"RK",			2, 2, READ, KILL,		msg,		ReadOperands },
	{ pUSB,	"RM",			2, 2, READ, MINE,		msg,		ReadOperands },
	{ pA,	"RH",			2, 2, READ, HEADER,		msg,		ReadOperands },
	{ pA,	"RW",			2, 2, READ, WHO,		msg,		ReadOperands },
	{ pUS,	"SB",			2, 2, SEND, BULLETIN,	msg,		SendOperands },
	{ pUS,	"SD",			2, 2, SEND, DISTRIB,	msg,		SendOperands },
	{ pUS,	"SEARCH",		3, 6, SEARCH, 0,		callbk,		DummyOperands },
	{ pUS,	"SHELL",		5, 5, SHELL, 0,		unixlogin,	DummyOperands },
	{ pUS,	"SIGNATURE",	3, 9, SIGNATURE, 0,	signature,	SignOperands },
	{ pUS,	"SI",			2, 2, SEND, MAIL,		msg,		SendOperands },
	{ pUSB,	"SKIP",			2, 4, EXCLUDE, 0,	usr,		InclOperands },
	{ pUSB,	"SOFTWARE",		2, 8, SOFTWARE, 0,	usr,		DelOperands },
	{ pUS,	"SP",			2, 2, SEND, PERSONAL,	msg,		SendOperands },
	{ pUS,	"SR",			2, 2, REPLY, 0,		msg,		DummyOperands },
	{ pUS,	"ST",			2, 2, SEND, NTS,		msg,		SendOperands },
	{ pUS,	"SS",			2, 2, SEND, SECURE,		msg,		SendOperands },
	{ pU,	"SYSOP",		5, 5, SYSOP, 0,		maint,		DummyOperands },
	{ pUSB,	"OWI21957",		8, 8, SYSOP, HARD,		maint,		DummyOperands },
	{ pUS,	"SEND",			1, 4, SEND, 0,		msg,		AllSendOperands },
	{ pUS,	"TALK",			2, 4, CHAT, 0,		chat,		DummyOperands },
	{ pUSB,	"TNC",			3, 3, TNC, 0,		usr,		DelOperands },
	{ pUS,	"TIMER",		2, 5, TIMER, 0,		maint,		OnOffOperands },
	{ pUSB,	"UPPERCASE",	2, 9, UPPERCASE, 0,	usr,		OnOffOperands },
	{ pUSB,	"UUSTAT",		3, 6, UUSTAT, 0,		uustat,		DummyOperands },
#ifdef OLD_CMDS
	{ pUSB,	"UUCP",			2, 4, EMAIL, 0,		usr,		EmailOperands },
#endif
	{ pA,	"USERS",		1, 5, USER, 0,		usr,		UserOperands },
	{ pUS,	"VACATION",		2, 8, VACATION, 0,	vacation,	SignOperands },
	{ pA,	"VERSION",		2, 7, VERSION, 0,	maint,	DummyOperands },
#if 0
	{ pS,	"VOICE",		5, 5, VOICE, 0,		voice_t,	DummyOperands },
#endif
	{ pA,	"WHEREIS",		3, 7, WHEREIS, 0,	wp,			DummyOperands },
	{ pA,	"WHO",			3, 3, WHO, 0,		usr,		DummyOperands },
	{ pA,	"WP",			2, 2, WP, 0,			wp,			WpOperands },
	{ pUS,	"WRITE",		2, 5, WRITE, 0,		filesys,	WriteOperands },
	{ pA,	"WX",			2, 2, WX, 0,			wx,			WxOperands },
	{ pUSB,	"ZIP",			1, 3, ZIP, 0,		usr,		DummyOperands },
	{ pUSB,	"0",			1, 1, MACRO, 0,		cmd,		DummyOperands },
	{ pUSB,	"1",			1, 1, MACRO, 0,		cmd,		DummyOperands },
	{ pUSB,	"2",			1, 1, MACRO, 0,		cmd,		DummyOperands },
	{ pUSB,	"3",			1, 1, MACRO, 0,		cmd,		DummyOperands },
	{ pUSB,	"4",			1, 1, MACRO, 0,		cmd,		DummyOperands },
	{ pUSB,	"5",			1, 1, MACRO, 0,		cmd,		DummyOperands },
	{ pUSB,	"6",			1, 1, MACRO, 0,		cmd,		DummyOperands },
	{ pUSB,	"7",			1, 1, MACRO, 0,		cmd,		DummyOperands },
	{ pUSB,	"8",			1, 1, MACRO, 0,		cmd,		DummyOperands },
	{ pUSB,	"9",			1, 1, MACRO, 0,		cmd,		DummyOperands },
	{ pUSB,	"?",			1, 1, HELP, 0,		help,		HelpOperands },
	{ pUS,	"[",			1, 1, BBSSID, 0,		usr,		DummyOperands },
	{ pA,	"*",			1, 1, BYE, 0,		exitbbs,	DummyOperands },
	{ pA,	";",			1, 1, BREAK, 0,			NULL, NULL },
	{ pA,	 NULL,			0, 0, 0, 0,		NULL,		DummyOperands }};
