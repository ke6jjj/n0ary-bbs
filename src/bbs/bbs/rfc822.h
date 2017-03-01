extern int
	rfc822_display_held(int num),
	rfc822_display_field(int num, int token),
	rfc822_skip_to(FILE *fp),
	rfc822_append_tl(int num, int token, struct text_line *tl),
	rfc822_append(int num, int token, char *s);

extern char
	*rfc822_get_field(int num, int token);
