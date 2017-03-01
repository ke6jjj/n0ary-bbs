
struct event_entry {
	time_t time;
	int number;
	char title[40];
	char location[40];
	char keyword[60];
};

#define	SizeofEvent	sizeof(struct event_entry)

extern int
	event_aging(void),
	event_t(void);
