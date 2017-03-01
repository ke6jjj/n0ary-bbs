int signature(void);
void kill_signature(void);
int make_signature(void);
int show_signature(void);
int vacation(void);
void kill_vacation(void);
int make_vacation(void);
int show_vacation(void);
int vacation_file_exists(void);
int signature_file_exists(void);
int filesys(void);
void file_init(void);
int file_cd(struct TOKEN *t);
int file_ls(struct TOKEN *t);
int file_approve(struct TOKEN *t);
int information(void);
int file_write_t(struct TOKEN *t);
int file_read_t(struct TOKEN *t);
int illegal_directory(char *str);
int get_file_body(FILE *fp);

#ifdef NOMSGD
void filesys_server(int msg_num, char *path);
#endif

int filesys_write_msg(int msgnum, char *path);
