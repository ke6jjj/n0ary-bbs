#include <time.h>
#include "smtp.h"
#include "alib.h"

#define Error(s)	"NO, "s"\n"
#define Ok(s)		"OK, "s"\n"
#define JustOk		"OK\n"
#define NoResponse	""

struct Ports {
	struct Ports *next;
	char *name;
	int lock;
	int lockable;
	char *reason;
};

extern struct Ports *PortList;

struct active_processes {
	struct active_processes *next;
	int proc_num;
	int fd;
	int pid;
	long t0;
	long idle;
	struct Ports *via;
	int chat;
	int chat_port;
	int notify;
	int verbose;
	char *text;
	char call[10];
	char buf[1024];
	size_t sz;
	alEventHandle ev;
};

extern struct active_processes *procs;

extern struct text_line *Notify;

extern int
	Port;

extern char
	output[4096];

extern long
	bbs_time(long *t);

extern int
	config_fetch_list(char *token),
	read_config_file(char *fn),
	config_fetch_multi(char *token, struct text_line **tl),
	config_override(char *token, char *value),
	daemon_check_in(struct active_processes *ap),
	daemon_check_out(struct active_processes *ap),
	daemons_start(void),
	daemons_check(void);

extern char
	*parse(struct active_processes *ap, char *s),
	*config_fetch(char *token),
	*config_fetch_orig(char *token);

extern void
	daemon_respawn(char *name, int state),
	lock_all(char *reason),
	lock_clear_all(void),
	lock_init(void);

extern struct Ports
	*locate_port(char *via);
