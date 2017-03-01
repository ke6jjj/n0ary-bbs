#include <time.h>

#define Error(s)	"NO, "s"\n"
#define Ok(s)		"OK, "s"\n"

struct active_processes {
	struct active_processes *next;
	char cmd[256];
	int fd;
	int sock;
	int listen;
};

extern int
	dbug_level;

extern int
    Msgd_Port, Userd_Port, Wpd_Port, Bidd_Port, Gated_Port,
	Tcpd_Port;

extern char
	*Bin_Dir,
	*Bbs_Host;

extern void
	uppercase(char *s);

extern char
	*show_status(struct active_processes *ap),
	*get_string(char **str),
	*parse(struct active_processes *ap, char *s);
