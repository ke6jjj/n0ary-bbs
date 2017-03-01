#include <time.h>
#include "smtp.h"

#define Error(s)	"NO, "s"\n"
#define Ok(s)		"OK, "s"\n"

#define dbgNOCONNECTBACK	2

extern int operation;
extern long new_level;
extern long bbs_mode;

extern time_t
	Time(time_t *t),
	time_now,
	Wpd_Refresh,
	Wpd_Age,
	Wpd_Update;

extern int
	dbug_level,
	Wpd_Update_Size,
	Wpd_Hour,
	Wpd_Port;

extern char
	*Bbs_Call,
	*Bbs_Host,
	*Bin_Dir,
	*Wpd_Global_Server,
	*Wpd_Local_Distrib,
	*Wpd_Update_Subject,
	*Wpd_Dump_File,
	*Wpd_User_File,
	*Wpd_Bbs_File;

struct active_processes {
	struct active_processes *next;
	struct wp_user_entry *wpu;
	struct wp_bbs_entry *wpb;
	int fd;
};

struct wp_user_entry {
	struct wp_user_entry *next;
	long	level;
	time_t	firstseen, seen, changed, last_update_sent;
	char	call[LenCALL];
	char	home[LenCALL];
	char	zip[LenZIP];
	char	fname[LenFNAME];
	char	qth[LenQTH];
};

struct wp_bbs_entry {
	struct wp_bbs_entry *next;
	long	level;
	char	call[LenCALL];
	char	hloc[LenHLOC];
};

#define	SizeofWpUser	sizeof(struct wp_user_entry)
#define	SizeofWpBBS		sizeof(struct wp_bbs_entry)

#define WP_Sysop		0
#define WP_User			1
#define WP_Guess		2
#define WP_Init			3

#define WriteALL		0
#define WriteUSER		1
#define WriteBBS		2

extern int
	user_image, bbs_image;

extern char
	output[4096];

#define CLEAN	0
#define DIRTY	1

extern char
	*get_call(char **str),
	*get_string(char **str),
	*get_string_to(char **str, char term);

extern long
	get_hexnum(char **str),
	get_number(char **str);

extern void
	startup(void),
	search(int fd, char *cmd),
	hash_search_user(int fd, char *pat, int mode),
	hash_search_bbs(int fd, char *pat),
	hash_user_init(void),
	hash_bbs_init(void);

extern struct wp_user_entry
	*make_user(char *call),
	*hash_create_user(char *s),
	*hash_get_user(char *s);

extern struct wp_bbs_entry
	*make_bbs(char *call),
	*hash_create_bbs(char *s),
	*hash_get_bbs(char *s);

extern int
	generate_wp_update(struct active_processes *ap),
	msg_generate(struct smtp_message *msg),
	disp_update(struct active_processes *ap, struct smtp_message *msg),
	read_new_user_file(char *filename),
	read_user_file(char *filename),
	read_new_bbs_file(char *filename),
	read_bbs_file(char *filename),
	hash_gen_update(FILE *gfp, int *gcnt, FILE *lfp, int *lcnt),
	hash_deleted_user(char *s),
	hash_deleted_bbs(char *s);

extern char
	*upload(char *cmd),
	*update(char *cmd),
	*update_read(int msg),
	*set_level(struct active_processes *ap, int token, char *s),
	*edit_string(struct active_processes *ap, int token, char *s),
	*edit_number(struct active_processes *ap, int token, char *s),
	*create_user(struct active_processes *ap, char *s),
	*kill_user(char *s),
	*show_user(struct active_processes *ap),
	*report_update(struct active_processes *ap),
	*write_file(char *cmd),
    *write_user_file(void),
    *write_bbs_file(void),
	*time2date(time_t t),
	*time2udate(time_t t),
	*hash_write(FILE *fp, int mode),
    *hash_bbs_cnt(void),
    *hash_user_cnt(void),
	*report_entry(char *call),
	*parse(struct active_processes *ap, char *s);

extern time_t
	Time(time_t *t),
	date2time(char *s),
	udate2time(char *s);
