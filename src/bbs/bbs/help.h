#define	SystemMenu	1
#define	MessageMenu	2
#define	FileMenu	4
#define	UserMenu	8
#define	SysopMenu	0x10
#define	AllMenus	(SystemMenu|MessageMenu|FileMenu|UserMenu|SysopMenu)

int error_string(int errmsg, char *str);
int error_number(int errmsg, int n);

