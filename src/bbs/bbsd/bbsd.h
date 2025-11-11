#include <sys/queue.h>
#include <time.h>
#include "smtp.h"
#include "alib.h"

#define Error(s)	"NO, "s"\n"
#define Ok(s)		"OK, "s"\n"
#define JustOk		"OK\n"
#define NoResponse	""

struct Port {
	SLIST_ENTRY(Port) entries;
	char *name;
	int lock;
	int lockable;
	char *reason;
};

SLIST_HEAD(port_list, Port);
extern struct port_list PortList;

struct active_process {
	LIST_ENTRY(active_process) entries;
	int proc_num;
	int fd;
	int pid;
	long t0;
	long idle;
	struct Port *via;
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

LIST_HEAD(proc_list, active_process);
extern struct proc_list procs;

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
	daemon_check_in(struct active_process *ap),
	daemon_check_out(struct active_process *ap),
	daemons_start(void),
	daemons_check(void);

extern char
	*parse(struct active_process *ap, char *s),
	*config_fetch(char *token),
	*config_fetch_orig(char *token);

extern void
	daemon_respawn(char *name, int state),
	lock_all(char *reason),
	lock_clear_all(void),
	lock_init(void);

extern struct Port
	*locate_port(char *via);
