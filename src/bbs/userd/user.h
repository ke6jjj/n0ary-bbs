#include <time.h>
#include "smtp.h"

#define Error(s)    "NO, "s"\n"
#define Ok(s)       "OK, "s"\n"

extern int
	dbug_level,
	Userd_Port;

extern char
	*Bbs_Call,
	*Bbs_Host,
	*Userd_Acc_Path;

time_t
	Time(time_t *t);
extern time_t
	time_now,
	Userd_Age_Suspect,
	Userd_Age_Home,
	Userd_Age_NotHome,
	Userd_Age_NonHam;

extern int operation;
extern char output[4096];

#define Error(s)	"NO, "s"\n"

struct UserDirectory {
	struct UserDirectory *next, *prev;
	char call[LenCALL];
	int class;
	int number;
	int immune;
	int refresh_day;
	time_t lastseen;
	int port;
	int connect_cnt;
};

#define SizeOfUserDir	sizeof(struct UserDirectory)

extern struct UserDirectory *UsrDir;

struct IncludeList {
	struct IncludeList *next;
	char	str[80];
};

struct Ports {
	struct Ports *next;
	char name[10];
	long	allow;
	long	firstseen;
	long	lastseen;
	long	count;
};

struct UserInformation {
	long	sysop, approved, nonham, help, ascending;
	long	newline, logging, immune, halfduplex;
	long	signature, vacation, email, emailall, regexp;
	long	uppercase, base, number, bbs, message;
	long	lines;

	char	email_addr[LenEMAIL];
	char	freq[LenFREQ];
	char	lname[LenLNAME];
	char	phone[LenPHONE];
	char	tnc[LenEQUIP];
	char	computer[LenEQUIP];
	char	rig[LenEQUIP];
	char	software[LenEQUIP];
	struct IncludeList *Include;
	struct IncludeList *Exclude;
	char	macro[10][LenMACRO];

	long	spare0;

	struct Ports *port;

	char	spare[256];
};

#define SizeOfUserInfo	sizeof(struct UserInformation)

struct active_processes {
	struct active_processes *next;
	struct UserList *ul;
	int fd;
};

struct ProcList {
	struct ProcList *next;
	struct active_processes *ap;
};

struct UserList {
	struct UserList *next;
	struct UserInformation info;
	char call[LenCALL];
	int condition;
	struct ProcList *pl;
};

#define CLEAN	0
#define DIRTY	1

void
	usrdir_immune(char *call, int immune),
	usrdir_number(char *call, int number),
	usrdir_touch(char *call, int port, long now),
	usrdir_allocate(char *call, char *s);

int
	usrdir_kill(char *s),
	usrdir_build(void),
	usrfile_write(char *call),
	usrdir_unique_number(int num),
	usrfile_create(char *call),
	usrfile_kill(char *call),
	usrfile_close_all(struct active_processes *ap),
	usrfile_close(struct active_processes *ap);

char
	*usrfile_show(void),
	*login_user(struct active_processes *ap, char *s),
	*edit_toggle(struct active_processes *ap, int token, char *s),
	*edit_string(struct active_processes *ap, int token, char *s),
	*edit_list(struct active_processes *ap, int token, char *s),
	*edit_number(struct active_processes *ap, int token, char *s),
	*disp_user(char *s),
	*find_user(char *s),
	*find_user_by_suffix(char *s),
	*age_users(struct active_processes *ap),
	*kill_user(char *s),
	*create_user(struct active_processes *ap, char *s),
	*show_user(struct active_processes *ap),
	*parse(struct active_processes *ap, char *s);

struct UserList
	*usrfile_find(char *call);

struct UserInformation
	*usrfile_read(char *call, struct active_processes *ap);

struct IncludeList
	*free_list(struct IncludeList *list),
	*add_2_list(struct IncludeList *list, char *str);

