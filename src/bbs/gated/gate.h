#include <time.h>
#include "smtp.h"

#define Error(s)	"NO, "s"\n"
#define Ok(s)		"OK, "s"\n"
#define JustOk		"OK\n"

struct active_processes {
	struct active_processes *next;
	int fd;
};

struct gate_entry {
	struct gate_entry *next;	/* next entry for this user */
	struct gate_entry *chain;
	struct gate_entry *hash;	/* next link for address hash bucket */
	char call[LenCALL];
	char addr[LenEMAIL];
	time_t seen;
	time_t warn_sent;
};

extern struct gate_entry
	*GateList, *GateHash[128];

extern int
	dbug_level,
	image;

extern time_t
	time_now,
	Time(time_t *t),
	Gated_Age_Warn,
	Gated_Age_Kill;

extern char
	output[4096];

#define CLEAN	0
#define DIRTY	1

extern int
	Gated_Port;

extern char
	*Bbs_Host,
	*Gated_File;

extern int
	delete_entry(char *call, char *addr),
	hash_key(char *s),
	add_user(struct gate_entry *g),
	add_address(struct gate_entry *g),
	read_file(void),
	write_file(void),
	write_if_needed(void);

extern char
    *time2date(time_t t),
	*disp_stat(void),
	*disp_guess(char *s),
	*parse(struct active_processes *ap, char *s);

extern void
    age(struct active_processes *ap),
	check_users(struct active_processes *ap, int create),
    read_new_file(void);
