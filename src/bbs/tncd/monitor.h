int monitor_init(char *bind_addr, int bind_port);
int monitor_shutdown(void);
int monitor_enabled(void);
void monitor_write(char *s, int me);
