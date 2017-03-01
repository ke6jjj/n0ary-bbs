int msg_read_t(struct TOKEN *t);
int msg_title_write(int num, FILE *fp);
int msg_revfwd(void);
int msg_forward(
	int (*cmd_upcall)(char *buf),
	int (*body_upcall)(char *buf),
	int (*term_upcall)(void));
void read_messages(struct TOKEN *t);
