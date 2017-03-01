#include "alib.h"

struct Tnc {
	int inuse;
	int fd;
	char tty[80];
	void (*read_cb)(int dev, void *arg);
	void *read_arg;
	alEventHandle evHandle;
};

#define MAX_TNC		4
extern struct Tnc tnc[MAX_TNC];
