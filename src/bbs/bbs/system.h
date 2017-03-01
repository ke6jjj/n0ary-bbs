
struct System_alias {
	struct System_alias *next;
	char alias[20];
};

struct System_chat {
	struct System_chat *next;
	int dir;
#			define chatRECV		1
#			define chatSEND		2
#			define chatDELAY	3
#			define chatDONE		0
	long to;
	char txt[80];
};

struct System {
	struct System *next;
	struct System_alias *alias;
	struct System_chat *chat;

	char connect[80];

	int dow;
#		define dowSUNDAY		1
#		define dowMONDAY		2
#		define dowTUESDAY		4
#		define dowWEDNESDAY		8
#		define dowTHURSDAY		0x10
#		define dowFRIDAY		0x20
#		define dowSATURDAY		0x40
#		define dowEVERYDAY		0xFF

	int t0, t1;
	int now;

	char *port;
	char *sid;
	char *order;

	int options;
#		define optPOLL			1
#		define optSMTPMUL		2
#		define optREVFWD		4
#		define optDUMBTNC		8
#		define optNOHLOC		0x10

	int age;

	struct ax25_params ax25;
};

extern struct System *SystemList;

extern int
	system_valid_alias(char *name),
	system_my_alias(char *call, char *name),
	system_chk(void),
	system_open(void);

extern void
	system_close(void);

extern char
    *system_get_exp_sid(char *alias),
	*system_disp_alias(char *name);
