#include <time.h>
#include "smtp.h"

#define Error(s)	"NO, "s"\n"
#define Ok			"OK\n"

extern int
	PortNumber,
	dbug_level;

extern char
	*HostName,
	*LogFile;

struct processes {
	struct processes *next;
	time_t t0, t1;
	char whoami[80];
	struct text_line *txt;
	int fd;
};

