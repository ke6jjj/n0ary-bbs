struct MessageQueue {
	struct MessageQueue *next;
	int number;
	char alias[20];
};

extern struct MessageQueue *queue;


extern int
	fwddir_touch(int type, int number, char *alias),
	fwddir_kill(int number, char *alias),
	fwddir_check(int number),
	fwddir_queue_messages(struct System_alias *name, char *order);

extern void
	fwddir_kill_stamp(char *fn),
	fwddir_queue_free(void),
	fwddir_open(void),
	fwddir_close(void);
