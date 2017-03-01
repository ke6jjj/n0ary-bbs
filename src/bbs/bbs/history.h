
extern int
	cmd_number,
	history(void);

extern void
	history_init(int depth),
	history_add(char *cmd);

extern char
	*history_cmd(int num),
	*history_last_cmd(void);
