
#define Error(s)	"NO, "s"\n"
#define Ok(s)		"OK, "s"\n"

#define DBG_MSGTRANS	2
#define DBG_MSGFWD		4

extern int
	Msgd_Port;

extern char
	*Bbs_Call,
	*Msgd_Body_Path,
	*Msgd_Archive_Path,
	*Msgd_Fwd_Dir,
	*Msgd_Route_File,
	*Msgd_Group_File,
	*Msgd_System_File;

extern int dbug_level;

extern int operation;
extern long new_level;
extern long bbs_mode;

struct active_processes {
	struct active_processes *next;
	int fd;

	char call[20];
	struct groups *grp;
	int disp_mode;
#define dispBINARY		1
#define dispNORMAL		2
#define dispVERBOSE		3
	long list_sent;
	long list_mode;
#define NormalMode      0
#define MineMode        1
#define SysopMode       2
#define BbsMode			3
};

extern struct active_processes
	*procs;

extern int image;

extern char
	output[4096];

#define CLEAN	0
#define DIRTY	1

int
	build_msgdir(void);

struct message_list {
	struct message_list *next;
	struct msg_dir_entry *msg;
};

struct groups {
	struct groups *next;
	char name[20];
	int cnt;
	struct message_list *list;
};

#define PERS	0
#define BULL	1
#define NTS		2

#define TIME	(Time(NULL) + 2)

extern time_t
	time_now,
	Time(time_t *t),
	Msgd_Age_Interval,
	Msgd_Age_Active[3],
	Msgd_Age_Killed[3],
	Msgd_Age_Old;

extern struct msg_dir_entry
	*get_message(int number),
	*append_msg_list(void),
	*unlink_msg_list(struct msg_dir_entry *msg),
	*MsgDir;

extern struct groups
	*find_group(char *name),
	*GrpList;

extern void
	SetMsgActive(struct msg_dir_entry *m),
	SetMsgOld(struct msg_dir_entry *m),
	SetMsgHeld(struct msg_dir_entry *m),
	SetMsgKilled(struct msg_dir_entry *m),
	SetMsgImmune(struct msg_dir_entry *m),
	SetMsgRefresh(struct msg_dir_entry *m),
	SetMsgPersonal(struct msg_dir_entry *m),
	SetMsgBulletin(struct msg_dir_entry *m),
	SetMsgNTS(struct msg_dir_entry *m),
	SetMsgSecure(struct msg_dir_entry *m),
	SetMsgPassword(struct msg_dir_entry *m, char *s),
	SetMsgPending(struct msg_dir_entry *m),
	ClrMsgPending(struct msg_dir_entry *m),
	SetMsgNoForward(struct msg_dir_entry *m),
	SetMsgNoFwd(struct msg_dir_entry *m),
	SetMsgLocal(struct msg_dir_entry *m),
	SetMsgCall(struct msg_dir_entry *m),
	SetMsgCategory(struct msg_dir_entry *m),
	SetMsgRead(struct msg_dir_entry *m, char *call),
    fwddir_open(void),
    fwddir_close(void),
    free_systems(void),
    free_msgdir(void),
    clean_users(void),
    append_group(char *name, struct msg_dir_entry *msg),
    fwddir_kill_stamp(char *fn),
    pending_fwd_num(struct active_processes *ap, int num),
    pending_fwd(struct active_processes *ap, char *call, char msgtype),
	msg_body_kill(int num),
    list_messages(struct active_processes *ap),
	close_message(FILE *fp),
    check_route(struct active_processes *ap, char *s),
	read_who(struct active_processes *ap, struct msg_dir_entry *msg),
	read_message(int key, struct active_processes *ap, struct msg_dir_entry *msg, char *by),
	read_message_rfc(struct active_processes *ap, struct msg_dir_entry *msg),
    show_message(struct active_processes *ap, struct msg_dir_entry *msg),
	show_groups(struct active_processes *ap),
    remove_from_groups(struct msg_dir_entry *msg),
	build_list_text(struct msg_dir_entry *msg),
	fwd_stats(void),
    type_stats(void),
	age_messages(void);

extern char
	*rfc822_get_field(int num, int token),
	*edit_message(struct active_processes *ap, struct msg_dir_entry *msg, char *s),
	*msg_stats(void),
    *build_display(struct active_processes *ap, struct msg_dir_entry *msg),
	*parse(struct active_processes *ap, char *s);

extern int
	compress_messages(struct active_processes *ap),
	fwddir_rename(int orig, int new),
	msg_body_rename(int orig, int new),
	fwddir_check(int number),
    fwddir_kill(int number, char *alias),
    fwddir_touch(int type, int number, char *alias),
    set_forwarding(struct active_processes *ap, struct msg_dir_entry *m, int check),
    set_groups(struct msg_dir_entry *m),
    visible_by(struct active_processes *ap, struct msg_dir_entry *msg),
    read_by_rcpt(struct msg_dir_entry *msg),
    read_by_me(struct msg_dir_entry *msg, char *call),
    send_message(struct active_processes *ap),
    is_number(char *s),
	rfc822_display_held(struct active_processes *ap, int num),
	rfc822_display_field(int num, int token),
	rfc822_skip_to(FILE *fp),
	rfc822_append_tl(int num, int token, struct text_line *tl),
	rfc822_append(int num, int token, char *s),
	rfc822_append_complete(int num, char *s),
	rfc822_decode_fields(struct msg_dir_entry *m);

extern FILE
	*open_message(int num),
	*open_message_write(int num);
