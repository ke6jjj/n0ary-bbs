struct Tnc {
	int inuse;
	int fd;
	char tty[80];
};

#define MAX_TNC		4
extern struct Tnc tnc[MAX_TNC];
