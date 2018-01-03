

extern void
    display_tokens(void);

struct TOKEN {
	struct TOKEN *next, *last;
	int		token;
	int		auxtoken;
	int		location;
	char	lexem[1024];
	int		value;
	int		(*target_func)();
};

struct possible_tokens {
	int	allowed;
#		define pUSER		1
#		define pSYSOP		2
#		define pBATCH		4
#		define pRESTRICT	8
	char	*str;
	int	min, max;
	int	token, auxtoken;
	int	(*func)();
	struct possible_tokens *op_tbl;
};

extern struct possible_tokens
	ActOperands[],
	AddOperands[],
	AllSendOperands[],
	BatchOpcodes[],
	CallOperands[],
	CheckOperands[],
	ClearOperands[],
	DebugOperands[],
	DelOperands[],
	DistribOperands[],
	DummyOperands[],
	EditOperands[],
	EventOperands[],
	FilesysOperands[],
	HelpOperands[],
	InclOperands[],
	InitiateOperands[],
	KillOperands[],
	ListOperands[],
	LoadOperands[],
	MacroOperands[],
	MaintOpcodes[],
	NameOperands[],
	OnOffOperands[],
	Opcodes[],
	PortsOperands[],
	ReadOperands[],
	RestrictedOpcodes[],
	RListOperands[],
	SendOperands[],
	SetOperands[],
	SignOperands[],
	UserOperands[],
	WriteOperands[],
	WxOperands[];


extern void
	remove_token(struct TOKEN *t);
	
extern struct TOKEN
	*grab_token_struct(struct TOKEN *prev),
	*TokenList;

extern int
	parse_command_line(char *str);

extern char
	CmdLine[1024];

#define DBG_OFF				0
#define DBG_TOKENS			1
#define DBG_MSGTRANS		2
#define DBG_MSGFWD			4
#define DBG_USER			8
#define DBG_MSGLIST			0x10
#define DBG_NEW				0x800

#define UNKNOWN		0

/* OPCODE / OPERAND TOKENS */

#define ABOUT		1
#define	ACTIVATE	(ABOUT+1)
#define	ASCEND		(ACTIVATE+1)
#define	ADD		(ASCEND+1)
#define	ADDRESS		(ADD+1)
#define	AGE		(ADDRESS+1)
#define	ADVENTURE	(AGE+1)
#define	ALL		(ADVENTURE+1)
#define	ALTERED		(ALL+1)
#define	APPROVED	(ALTERED+1)
#define	AT		(APPROVED+1)
#define	ATBBS		(AT+1)
#define	ATDIST		(ATBBS+1)
#define	ATNTS		(ATDIST+1)
#define AXRELAY         (ATNTS+1)

#define	BACKSLASH	(AXRELAY+1)
#define	BASE	(BACKSLASH+1)
#define	BAROMETER	(BASE+1)
#define	BBS		(BAROMETER+1)
#define	BBSSID		(BBS+1)
#define	BID		(BBSSID+1)
#define	BREAK		(BID+1)
#define	BULLETIN	(BREAK+1)
#define	BYE		(BULLETIN+1)

#define	CALL		(BYE+1)
#define	CALLBK		(CALL+1)
#define	CATCHUP		(CALLBK+1)
#define	CD		(CATCHUP+1)
#define	CHANGE		(CD+1)
#define	CHAT		(CHANGE+1)
#define	CHECK		(CHAT+1)
#define	CLEAR		(CHECK+1)
#define	COMMA		(CLEAR+1)
#define	COMMANDS	(COMMA+1)
#define	COMPRESS	(COMMANDS+1)
#define	COMPUTER	(COMPRESS+1)
#define	CONSOLE		(COMPUTER+1)
#define	COUNT		(CONSOLE+1)
#define	COPY		(COUNT+1)
#define	CUSTOM		(COPY+1)

#define	DAYS		(CUSTOM+1)
#define	DASH		(DAYS+1)
#define	DATA		(DASH+1)
#define	DATE		(DATA+1)
#define	DEBUG		(DATA+1)
#define	DELETE		(DEBUG+1)
#define	DESCEND		(DELETE+1)
#define	DIRECTORY	(DESCEND+1)
#define	DISTRIB		(DIRECTORY+1)
#define	DUMP		(DISTRIB+1)
#define	DUPLEX		(DUMP+1)

#define	ECHOCMD		(DUPLEX+1)
#define	EDIT		(ECHOCMD+1)
#define	EMAIL		(EDIT+1)
#define	EQUIP		(EMAIL+1)
#define	EVENT		(EQUIP+1)
#define	EXCLUDE		(EVENT+1)
#define	EXPIRED		(EXCLUDE+1)

#define	FAST		(EXPIRED+1)
#define	FILESYS		(FAST+1)
#define	FIRST		(FILESYS+1)
#define	FIX			(FIRST+1)
#define	FNAME		(FIX+1)
#define	FORWARD		(FNAME+1)
#define	FREQ		(FORWARD+1)
#define	FRIDAY		(FREQ+1)
#define	FROM		(FRIDAY+1)
#define	FULL		(FROM+1)

#define GAMES           (FULL+1)
#define	GENERATE	(GAMES+1)
#define	GRAPH		(GENERATE+1)
#define	GROUP		(GRAPH+1)

#define	HARD		(GROUP+1)
#define	HDX		(HARD+1)
#define	HEADER		(HDX+1)
#define	HELD		(HEADER+1)
#define	HELP		(HELD+1)
#define	HISTORY		(HELP+1)
#define	HLOC		(HISTORY+1)
#define	HOLD		(HLOC+1)
#define	HOME		(HOLD+1)
#define	HUMIDITY	(HOME+1)

#define	IMMUNE		(HUMIDITY+1)
#define	INCLUDE		(IMMUNE+1)
#define	INFO		(INCLUDE+1)
#define	INITIATE	(INFO+1)
#define	IN		(INITIATE+1)
#define	INDOOR		(IN+1)

#define	KILL		(INDOOR+1)

#define	LAST		(KILL+1)
#define	LINES		(LAST+1)
#define	LIST		(LINES+1)
#define	LNAME		(LIST+1)
#define	LOAD		(LNAME+1)
#define	LOCAL		(LOAD+1)
#define	LOCALE		(LOCAL+1)
#define	LOCK		(LOCALE+1)
#define	LOG		(LOCK+1)
#define	LOOKUP		(LOG+1)
#define	LS		(LOOKUP+1)

#define	MACRO		(LS+1)
#define	MAIL		(MACRO+1)
#define	MAINT		(MAIL+1)
#define	ME		(MAINT+1)
#define	MESSAGE		(ME+1)
#define	MINE		(MESSAGE+1)
#define	MONDAY		(MINE+1)
#define	MONTHS		(MONDAY+1)
#define	MOTD		(MONTHS+1)

#define	NEW		(MOTD+1)
#define	NEWLINE		(NEW+1)
#define	NONE		(NEWLINE+1)
#define	NONHAM		(NONE+1)
#define	NOPROMPT	(NONHAM+1)
#define	NTS		(NOPROMPT+1)
#define	NUMBER		(NTS+1)

#define	OFF		(NUMBER+1)
#define	OLD		(OFF+1)
#define	ON		(OLD+1)
#define	OUT		(ON+1)

#define	PASSWORD	(OUT+1)
#define	PENDING		(PASSWORD+1)
#define	PERSONAL	(PENDING+1)
#define	PERIOD		(PERSONAL+1)
#define	PHONE		(PERIOD+1)
#define	PIPE		(PHONE+1)
#define	PORTS		(PIPE+1)
#define	PROCESS		(PORTS+1)
#define	PUNCTUATION	(PROCESS+1)

#define	QTH		(PUNCTUATION+1)

#define	RAIN		(QTH+1)
#define	READ		(RAIN+1)
#define	REFRESH		(READ+1)
#define	REGEXP		(REFRESH+1)
#define	RELEASE		(REGEXP+1)
#define	REMOTE		(RELEASE+1)
#define	REPLY		(REMOTE+1)
#define	RESTRICTED	(REPLY+1)
#define	REVFWD		(RESTRICTED+1)
#define	RIG		(REVFWD+1)
#define	ROUTE	(RIG+1)

#define	SATURDAY	(ROUTE+1)
#define	SEARCH		(SATURDAY+1)
#define	SECURE		(SEARCH+1)
#define	SEND		(SECURE+1)
#define	SET		(SEND+1)
#define	SHELL		(SET+1)
#define	SHOW		(SHELL+1)
#define	SIGNATURE	(SHOW+1)
#define	SINCE		(SIGNATURE+1)
#define	SIZE		(SINCE+1)
#define	SLASH		(SIZE+1)
#define	SLOW		(SLASH+1)
#define	SOFTWARE	(SLOW+1)
#define	SPAWN		(SOFTWARE+1)
#define	SSID		(SPAWN+1)
#define	SUBJECT		(SSID+1)
#define	SUNDAY		(SUBJECT+1)
#define	SUSPECT		(SUNDAY+1)
#define	SYSOP		(SUSPECT+1)
#define	STATUS		(SYSOP+1)
#define	STRING		(STATUS+1)

#define	TEMPERATURE	(STRING+1)
#define	THURSDAY	(TEMPERATURE+1)
#define	TIME		(THURSDAY+1)
#define	TIMER		(TIME+1)
#define	TO		(TIMER+1)
#define	TOCALL		(TO+1)
#define	TOGROUP		(TOCALL+1)
#define	TONTS		(TOGROUP+1)
#define	TOUCH		(TONTS+1)
#define	TNC		(TOUCH+1)
#define	TNC144		(TNC+1)
#define	TNC220		(TNC144+1)
#define	TNC440		(TNC220+1)
#define	TUESDAY		(TNC440+1)

#define	UNLOCK		(TUESDAY+1)
#define	UNREAD		(UNLOCK+1)
#define	UPPERCASE		(UNREAD+1)
#define	USER		(UPPERCASE+1)
#define	UUSTAT		(USER+1)

#define	VACATION	(UUSTAT+1)
#define	VERSION		(VACATION+1)
#define	VOICE		(VERSION+1)

#define	WAIT		(VOICE+1)
#define	WEDNESDAY	(WAIT+1)
#define	WEEKS		(WEDNESDAY+1)
#define	WHEREIS		(WEEKS+1)
#define	WHO		(WHEREIS+1)
#define	WIND		(WHO+1)
#define	WORD		(WIND+1)
#define	WP		(WORD+1)
#define	WRITE		(WP+1)
#define	WX		(WRITE+1)
#define	WHY		(WX+1)

#define	YEARS		(WHY+1)
#define	YESTERDAY	(YEARS+1)

#define	ZIP		(YESTERDAY+1)
#define	ZORK		(ZIP+1)

#define ACMD		(ZORK+1)
#define BCMD		(ACMD+1)
#define CCMD		(ACMD+2)
#define DCMD		(ACMD+3)
#define ECMD		(ACMD+4)
#define FCMD		(ACMD+5)
#define GCMD		(ACMD+6)
#define HCMD		(ACMD+7)
#define ICMD		(ACMD+8)
#define JCMD		(ACMD+9)
#define KCMD		(ACMD+10)
#define LCMD		(ACMD+11)
#define MCMD		(ACMD+12)
#define NCMD		(ACMD+13)
#define OCMD		(ACMD+14)
#define PCMD		(ACMD+15)
#define QCMD		(ACMD+16)
#define RCMD		(ACMD+17)
#define SCMD		(ACMD+18)
#define TCMD		(ACMD+19)
#define UCMD		(ACMD+20)
#define VCMD		(ACMD+21)
#define WCMD		(ACMD+22)
#define XCMD		(ACMD+23)
#define YCMD		(ACMD+24)
#define ZCMD		(ACMD+25)

#define END		(ACMD+26)
