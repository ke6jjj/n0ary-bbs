
#define	UserHelp0			0x0000001
#define	UserHelp1			0x0000002
#define	UserHelp2			0x0000004
#define	UserHelp3			0x0000008

struct IncludeList {
	struct IncludeList *next;
	struct IncludeList *comp;
	int key;
	char str[10];
};

extern struct IncludeList
	*Include,
	*Exclude;

extern char
	include[LenINCLUDE],
	exclude[LenINCLUDE],
	usercall[LenCALL];

extern time_t
	last_login_time;

extern long
	Lines,
	Base,
	ImLogging,
	ImSysop,
	ImBBS,
	ImNew,
	IGavePassword,
	ISupportHloc,
	ISupportBID;

extern long
	MyHelpLevel,
	ImSuspect,
	NeedsNewline,
	Cnvrt2Uppercase,
	ImAllowedSysop,
	ImAscending,
	ImDescending,
	ImHalfDuplex,
	ImRegExp,
	ImPotentialBBS,
	ImNonHam;

#define AutoVacation		user_check_flag(uVACATION)
#define AutoEMAILForward	user_check_flag(uEMAIL)
#define AutoSignature		user_check_flag(uSIGNATURE)

#if 0
#define ImSuspect			!user_check_flag(uAPPROVED)
#define NeedsNewline		user_check_flag(uNEWLINE)
#define	ImAllowedSysop		user_check_flag(uSYSOP)
#define ImAscending			user_check_flag(uASCEND)
#define ImDescending		!ImAscending
#define ImHalfDuplex		user_check_flag(uHDUPLEX)
#define ImRegExp			user_check_flag(uREGEXP)
#define ImPotentialBBS		user_check_flag(uBBS)
#define	ImNonHam			user_check_flag(uNONHAM)
#endif

extern int
	usr(void),
    parse_bbssid(char *str, char *sid),
	user_check_flag(int token),
	user_open(void),
	user_focus(char *call),
	user_get_list(int token),
	user_cmd(int token);

extern long
	user_get_value(int token);

extern char
	*usr_disp_list_entry(struct IncludeList *list),
	*user_by_number(int number),
	*user_get_macro(int number),
	*user_get_field(int token);

extern void
    user_flag(int token, char *toggle),
	fill_in_blanks(void),
	user_refresh(void),
	user_refresh_msg(void),
	usr_disp_gateallow(char *s),
    usr_disp_list(struct IncludeList *list),
    user_disp_account(char *call),
	user_login(char *via),
	user_create(void),
	user_clr_flag(int token),
	user_set_flag(int token),
    user_set_macro(int number, char *s),
	user_set_field(int token, char *s),
	user_set_value(int token, long val);

#if 0
struct user_information {
	long	flags;
	long	rt_flags;
	long	allowed;
	long	number;
	long	time_stamp;
	long	connect_via;
	long	fwd_mask;
	long	last_msg;
	char	spare2[20];
				/* White Pages Info */
	struct call_and_checksum call;
	char	fname[LenFNAME];
	char	qth[LenQTH];
	char	zip[LenZIP];
	char	homebbs[LenHOME];
				/* EMAIL forwarding address */
	char	email[LenEMAIL];
	char	freq[LenFREQ];
				/* Local Database Info */	
	char	lname[LenLNAME];
	char	phone[LenPHONE];
	char	tnc[LenEQUIP];
	char	computer[LenEQUIP];
	char	rig[LenEQUIP];
	char	software[LenEQUIP];
	char	spare3[20];
				/* List Categories Info */
	struct call_and_checksum
		include[20], exclude[20];	
				/* Terminal parameters */
	long	term;
	long	lines;
				/* User Parameters */
	char	macro[10][LenMACRO];
	char	password[LenPASSWD];
				/* History */
	char	padding[2];
	struct {
		long	firstseen;
		long	lastseen;
		long	count;
	} connect[6];

	long	counts[10];
				/* Runtime variables */
	time_t	last_activity;

	char	spare4[252];
};

#define	SizeofUser		sizeof(struct user_information)
extern struct user_information
	CurrentUser, *CurUser;

struct user_directory {
	char	call[7];
	long	number;
	long	last_seen;
	long	method;
	long	connect_cnt;
};

#define	SizeofUserDir		sizeof(struct user_directory)
extern struct user_directory
	*UserDir;
extern int UserDirCnt;

/* Methods of connection */
#define	ConTnc144			0
#define	ConTnc220			1
#define	ConTnc440			2
#define	ConPhoneFAST		3
#define	ConPhoneSLOW		4
#define	ConConsole			5


/* Count types */
#define	UserCallbookLookup	0
#define	UserCallbookSearch	1
#define	UserWx				2
#define	UserWxTrend			3
#define	UserEvent			4
#define	UserEventAdd		5
#define	UserSentNTS			6
#define	UserDeliveredNTS	7
#define	UserSpare1			8
#define	UserSpare2			9

/*==============*
 * Static flags *
 *==============*/

#define	UserSysop			0x0000010
#define	UserApproved		0x0000020
#define	UserSuspect			0x0000040
#define	UserNonHam			0x0000080
#define UserRestricted		0x0100000
#define UserTypeMask		0x01000F0

#define	UserHelp0			0x0000001
#define	UserHelp1			0x0000002
#define	UserHelp2			0x0000004
#define	UserHelp3			0x0000008
#define UserHelpMask		0x000000F

#define	UserMsgAscending	0x0001000
#define	UserNewlineReqd		0x0002000
#define	UserLogging			0x0004000
#define	UserImmune			0x0008000

#define UserHalfDuplex		0x0010000
#define UserAutoSignature	0x0020000
#define UserEMAILForward		0x0040000
#define UserVacation		0x0080000

#define UserRegExp			0x0100000

/*==================*/

#define	IsUserAllowedSysop(u)	(u->flags & UserSysop)
#define	ImAllowedSysop			(CurrentUser.flags & UserSysop)

#define	IsUserApproved(u)		(u->flags & UserApproved)
#define	ImApproved				(CurrentUser.flags & UserApproved)

#define	IsUserSuspect(u)		(u->flags & UserSuspect)
#define	ImSuspect				(CurrentUser.flags & UserSuspect)

#define	IsUserNonHam(u)			(u->flags & UserNonHam)
#define	ImNonHam				(CurrentUser.flags & UserNonHam)

#define	UserHelpLevel(u)		(ImBBS ? UserHelp0 :(u->flags & UserHelpMask))
#define	IsUserNovice(u)			(u->flags & UserHelp3)
#define	ImNovice				(UserHelpLevel(CurUser) & UserHelp3)
#define	ImNotNovice				(!(CurrentUser.flags & UserHelp3))

#define IsUserImmune(u)			(u->flags & UserImmnue)
#define	ImImmune				(CurrentUser.flags & UserImmune)

#define IsUserLogging(u)		(u->flags & UserLogging)
#define ImLogging				(CurrentUser.flags & UserLogging)
#define NeedsNewline			(CurrentUser.flags & UserNewlineReqd)

#define IsUserRestricted(u)		(u->flags & UserRestricted)
#define ImRestricted			(CurrentUser.flags & UserRestricted)

#define ImAscending				(CurrentUser.flags & UserMsgAscending)
#define ImDescending			(!(CurrentUser.flags & UserMsgAscending))
#define ImHalfDuplex			(CurrentUser.flags & UserHalfDuplex)
#define AutoSignature			(CurrentUser.flags & UserAutoSignature)
#define AutoVacation			(CurrentUser.flags & UserVacation)
#define AutoEMAILForward				(CurrentUser.flags & UserEMAILForward)
#define ImRegExp				(CurrentUser.flags & UserRegExp)
#endif

/*===============*
 * Runtime flags *
 *===============*/

#define	UserBBS				0x00000001
#define UserHloc			0x00000002
#define UserBID				0x00000004

#define	UserGavePassword	0x00001000

#define UserIsSysop			0x010000000

#define	UserIsNew_book		0x00020000
#define	UserIsNew_wp		0x00040000
#define	UserIsNew_unknown	0x00080000
#define UserIsNewMask		(UserIsNew_book|UserIsNew_wp|UserIsNew_unknown)

#if 0
#define	UserOnTnc144		0x00100000
#define	UserOnTnc220		0x00200000
#define	UserOnTnc440		0x00400000
#define	UserOnTnc		(UserOnTnc144|UserOnTnc220|UserOnTnc440)
#define	UserOnPhoneSLOW		0x01000000
#define	UserOnPhoneFAST		0x02000000
#define	UserOnPhone		(UserOnPhoneSLOW|UserOnPhoneFAST)
#define	UserOnConsole		0x08000000
#define	UserSecure		(UserOnConsole|UserOnPhone)
#define UserAccessMethod	(UserOnConsole|UserOnTnc|UserOnPhone)
#endif

/*==================*/

#define CREATE_USER		TRUE
#define NOCREATE_USER	FALSE

extern int
	user_allowed_on_port(char *via);

#if 0
/*usr_maint.c*/
extern int
	usr_count(void),
	read_user_directory(void),
	usr(void),
	usr_aging(int purge),
	verify_users_wp(void);

extern void
    user_flag(int token, char *toggle),
	fill_in_blanks(void),
	display_most_recent_connects(int cnt);

/*user.c*/
extern int
	set_usr(),
	update_user_file(struct user_information *u),
	fetch_user(char *call, struct user_information *u, int create),
	touch_user(char *call, long t),
	time_stamp_user(struct user_information *u, long int t),
	show_usr(void),
	assign_user_number(struct user_information *u);

/*usr_util.c*/
extern void
	SetUserAllowed(struct user_information *u, long int mask),
	ClrUserAllowed(struct user_information *u, long int mask),
	SetUserSysop(struct user_information *u),
	ClrUserSysop(struct user_information *u),
	SetUserApproved(struct user_information *u),
	SetUserSuspect(struct user_information *u),
	SetUserRestricted(struct user_information *u),
	ClrUserApproved(struct user_information *u),
	ClrUserSuspect(struct user_information *u),
	ClrUserRestricted(struct user_information *u),
	SetUserNonHam(struct user_information *u),
	ClrUserNonHam(struct user_information *u),
	SetUserImmune(struct user_information *u),
	ClrUserImmune(struct user_information *u),
	SetUserRegExp(struct user_information *u),
	ClrUserRegExp(struct user_information *u),
	SetUserSignature(struct user_information *u),
	ClrUserSignature(struct user_information *u),
	SetUserVacation(struct user_information *u),
	ClrUserVacation(struct user_information *u),
	SetUserAutoEMAILForward(struct user_information *u),
	ClrUserAutoEMAILForward(struct user_information *u),
	SetUserNewline(struct user_information *u),
	ClrUserNewline(struct user_information *u),
	SetUserMsgAscending(struct user_information *u),
	SetUserMsgDescending(struct user_information *u),
	SetUserEcho(struct user_information *u),
	ClrUserEcho(struct user_information *u),
	SetUserLogging(struct user_information *u),
	ClrUserLogging(struct user_information *u),
	SetUserGavePassword(struct user_information *u),
	SetUserBBS(struct user_information *u),
	ClrUserBBS(struct user_information *u),
	SetUserBBShloc(struct user_information *u),
	ClrUserBBShloc(struct user_information *u),
	SetUserBBSbid(struct user_information *u),
	ClrUserBBSbid(struct user_information *u),
	SetUserHelpLevel(struct user_information *u, int value);

extern char
	*cnvrt_usrnum_call(int num);

/*dump.c*/
extern int
	dump(void);

extern int get_user(struct user_information *u, char *call);

extern char
	*cnvrt_usrnum_call(int num);

/*usr_disp.c*/
extern void
    user_disp_account(char *call),
	usr_disp_list(struct IncludeList *list);

extern char
	*usr_disp_list_entry(struct IncludeList *list);

extern int
	display_user_struct(struct user_information *u, int me);
#endif 
/*========================*/

#define AlterUserApproved(u,s) if(s==SET) SetUserApproved(u); else ClrUserApproved(u)
#define AlterUserSuspect(u,s) if(s==SET) SetUserSuspect(u); else ClrUserSuspect(u)
#define AlterUserSysop(u,s) if(s==SET) SetUserSysop(u); else ClrUserSysop(u)
#define AlterUserNonHam(u,s) if(s==SET) SetUserNonHam(u); else ClrUserNonHam(u)
#define AlterUserImmune(u,s) if(s==SET) SetUserImmune(u); else ClrUserImmune(u)
#define AlterUserRegExp(u,s) if(s==SET) SetUserRegExp(u); else ClrUserRegExp(u)
#define AlterUserRestricted(u,s) if(s==SET) SetUserRestricted(u); else ClrUserRestricted(u)
#if 0
#define AlterUserMotdSeen(u,s) if(s==SET) SetUserMotdSeen(u); else ClrUserMotdSeen(u)
#endif
#define AlterUserNewline(u,s) if(s==SET) SetUserNewline(u); else ClrUserNewline(u)
#define AlterUserSignature(u,s) if(s==SET) SetUserSignature(u); else ClrUserSignature(u)
#define AlterUserVacation(u,s) if(s==SET) SetUserVacation(u); else ClrUserVacation(u)
#define AlterUserAutoEmailForward(u,s) if(s==SET) SetUserAutoEMAILForward(u); else ClrUserAutoEMAILForward(u)
#define AlterUserLogging(u,s) if(s==SET) SetUserLogging(u); else ClrUserLogging(u)
#define AlterUserMsgAscending(u,s) if(s==SET) SetUserMsgAscending(u); else SetUserMsgDescending(u)
#define AlterUserMsgDescending(u,s) if(s==SET) SetUserMsgDescending(u); else SetUserMsgAscending(u)
#define AlterUserBBS(u,s) if(s==SET) SetUserBBS(u); else ClrUserBBS(u)
#define AlterUserBBShloc(u,s) if(s==SET) SetUserBBShloc(u); else ClrUserBBShloc(u)
#define AlterUserBBSbid(u,s) if(s==SET) SetUserBBSbid(u); else ClrUserBBSbid(u)
#define AlterUserAllowed(u,m,s) if(s==SET) SetUserAllowed(u,m); else ClrUserAllowed(u,m)
#define AlterUserEcho(u,s) if(s==SET) SetUserEcho(u); else ClrUserEcho(u)

int logout_user(void);
