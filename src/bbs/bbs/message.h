#include <sys/types.h>

struct list_criteria {
	int		must_include_mask;
	int		include_mask;
	int		exclude_mask;
	int		range_type;
	int		range[2];
	int		pattern_type;
	time_t	since;
	short	pattern_sum[4];
	char 	pattern[4][20];
};

struct user_list_info {
	struct call_and_checksum call;
	long	number;
	long	last_msg;
};

#define MatchTO		0
#define MatchFROM	1
#define MatchAT		2
#define MatchSUB	3

#define MatchTOmask		1
#define MatchFROMmask	2
#define MatchATmask		4
#define MatchSUBmask	8

struct generated_message {
	struct generated_message *next;
	char call[10];
	struct text_line *body;
};

struct generated_message *
	buffer_msg_to_user(char *name, char *buf);

extern struct msg_dir_entry
	*MsgDirList;

extern int
	active_message,
	last_message_listed;

extern char
	Group[20];

#define	SizeofMsg	sizeof(struct message_directory_entry)

/*message.c*/
extern int
	msg(void),
	message(void),
	append_active_message(void);

extern struct TOKEN *
	match_address(struct TOKEN *t, struct msg_dir_entry *md);

/*msg_maint.c*/
extern int
	clean_message_directory(void);

extern int
	set_forwarding(struct msg_dir_entry *m, int check);

extern void
    msg_build_bid(void),
    msg_build_mid(void),
	field_translation(struct msg_dir_entry *m);

/*msg_util.c*/

extern void
	msg_forwarded(int num, char *alias),
	flush_message_list(void),
	SetMsgAborted(struct msg_dir_entry *m),
	SetMsgCreate(struct msg_dir_entry *m),
	SetMsgActive(struct msg_dir_entry *m),
	SetMsgOld(struct msg_dir_entry *m),
	SetMsgHeld(struct msg_dir_entry *m),
	SetMsgKilled(struct msg_dir_entry *m),
	SetMsgPersonal(struct msg_dir_entry *m),
	SetMsgBulletin(struct msg_dir_entry *m),
	SetMsgNTS(struct msg_dir_entry *m),
	SetMsgSecure(struct msg_dir_entry *m),
	SetMsgPassword(struct msg_dir_entry *m, char *s),
	SetMsgPending(struct msg_dir_entry *m),
	SetMsgInProcess(struct msg_dir_entry *m),
	SetMsgImmune(struct msg_dir_entry *m),
	SetMsgForwarded(struct msg_dir_entry *m),
	SetMsgNoForward(struct msg_dir_entry *m),
	SetMsgNoFwd(struct msg_dir_entry *m),
	SetMsgLocal(struct msg_dir_entry *m),
	SetMsgCheckOUt(void),
	SetMsgCheckIN(void),
	SetMsgCall(struct msg_dir_entry *m),
	SetMsgCategory(struct msg_dir_entry *m),
	SetMsgRead(struct msg_dir_entry *m),
	set_user_list_info(void),
	SetMsgRefresh(struct msg_dir_entry *m);

extern int
	alias_pending_cnt(char *alias),
	msg_get(struct msg_dir_entry *m, int num),
	msg_pending_cnt(int num),
	msg_edit_issue(int num, char *rfc),
	msg_issue(struct msg_dir_entry *m),
	set_listing_flags(struct msg_dir_entry *m),
	build_partial_list(struct list_criteria *lc),
	last_message_number(void),
	reserve_msg_directory_entry(struct msg_dir_entry *m),
	get_msg_directory_entry(struct msg_dir_entry *m),
	put_msg_direcotry_entry(void),
	last_message_number(void);

extern struct msg_dir_entry
	*msg_locate(int msgnum),
	*free_message_list(void),
	*build_full_message_list(void);

extern void
	check_for_recpt_options(struct msg_dir_entry *m);

/* msg_hold.c */
extern int
	msg_hold_release(int num),
	msg_hold_disp_reason(int num),
	msg_hold_get_reason(struct msg_dir_entry *m, char *reason),
	msg_hold_by_bbs(int num, char *reason);

/* msg_body.c */
extern FILE
	*open_message(int num);

extern void
    msg_free_body(struct msg_dir_entry *m),
	close_message(FILE *fp);

extern int
	copy_msg_bodies(int org, int dup);

extern int
	determine_hloc_for_bbs(struct msg_dir_entry *md),
	determine_homebbs(struct msg_dir_entry *md);

/* msg_send.c */

extern int
    msg_snd_t(struct TOKEN *head),
    msg_rply_t(struct TOKEN *head),
    msg_copy_t(struct TOKEN *head),
    msg_copy_parse(int number, char *to),
    msg_reject(struct msg_dir_entry *msg),
    msg_snd(struct msg_dir_entry *msg),
    msg_snd_dist(struct msg_dir_entry *msg, char *fn),
    msg_rply(int num),
    msg_copy(int num, struct msg_dir_entry *msg);
    
extern void
    check_for_recpt_options(struct msg_dir_entry *m),
    read_routing(char *buf, char *homebbs, time_t *orig_date, int *num);

int msg_xlate_tokens(struct TOKEN *t, struct msg_dir_entry *msg);
