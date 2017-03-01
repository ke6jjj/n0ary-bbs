
struct Monitor_Procs {
	struct Monitor_Procs *next;
	int fd;
	int disp;
#define DispNONE	0
#define DispALL		1
#define DispME		2
};

extern struct Monitor_Procs *MonProcs;

extern int monitor, monitor_sock;


extern int
	monitor_enabled(void);

extern void
	monitor_chk(void),
	monitor_control(void),
	monitor_write(char *s, int me);

extern struct Monitor_Procs
	*monitor_disc(struct Monitor_Procs *mp);
