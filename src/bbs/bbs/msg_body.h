int include_msg(char *buf, int type /*Possibly outdated arg*/);
int get_subject(struct msg_dir_entry *m);
int get_message_body(struct msg_dir_entry *m);
int gen_vacation_body(struct text_line **tl, char *from);
