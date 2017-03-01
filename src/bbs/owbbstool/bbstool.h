#define Decoration 14

#define NOWIN		0
#define LCKWIN		1
#define USRWIN		2
#define DMNWIN		3
#define MSGWIN		4

struct active_users {
	struct active_users *next;
	int proc_num;
	int chat_port;
	int chat;
	int window;
	OlListToken token;
	String label;
	long ctime;
	long idle;
	char call[10];
	char via[10];
	char msg[256];
	char display[256];
};

extern char
	*Bbs_Call;

extern struct active_users *ActiveUsers;

extern void
	proc_connect(Widget w, XtPointer client_data, XtPointer cb);

extern void
	usr_callback(Widget widget, caddr_t client_data, caddr_t call_data),
	lck_callback(Widget widget, caddr_t client_data, caddr_t call_data),
	dmn_callback(Widget widget, caddr_t client_data, caddr_t call_data);
