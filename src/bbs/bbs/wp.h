#if 0
struct wp_user_entry {
	long	date;
	char	call[LenCALL];
	long	how;
	char	homebbs[LenHOME];
	char	zip[LenZIP];
	char	fname[LenFNAME];
	char	qth[LenQTH];
	long	changed;
};

#define	SizeofWpUser	sizeof(struct wp_user_entry)
#endif

extern char
	*wp_fetch(char *cmd),
	*wp_get_field(char *call, int field);

extern int
	wp_create_user(char *call),
	wp_create_bbs(char *call),
	wp_fetch_multi(char *cmd),
	wp_cmd(char *cmd),
	wp_set_field(char *call, int field, int level, char *value),
	wp_home_count(char *call),
	wp_set_users_homebbs(char *call, char *homebbs, int level),
	wp_test_field(char *call, int token),
	wp(void);

extern void
	wp_seen(char *call);
