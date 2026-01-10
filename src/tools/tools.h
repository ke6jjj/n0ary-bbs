#include <inttypes.h>
#include <time.h>

#ifndef TIME_T_FORMAT_SET
#  ifdef __LP64__
#    define PRTMd PRId64
#  else
#    define PRTMd "d"
#  endif
#  define TIME_T_FIXED
#endif

#ifndef OFF_T_FORMAT_SET
#  ifdef __LP64__
#    define PROFFTd PRId64
#  else
#    define PROFFTd "lld"
#  endif
#  define OFF_T_FORMAT_SET
#endif

#define sockOK          0
#define sockTIMEOUT     1
#define sockMAXLEN      2
#define sockERROR       (-1)

struct text_line {
	struct text_line *next;
	char *s;
};

extern struct text_line *error_list;

struct tty {
	char *device;
	int mode;
	int oflag, lflag, cflag, iflag;
	int fndelay;
};

struct active_procs {
	struct active_procs *next;
	int fd;
};

struct active_procs
	*proc_add(void),
	*proc_remove(struct active_procs *ap);

struct text_line
	*textline_allocate(char *s),			/* textline.c */
	*textline_free(struct text_line *tl);	/* textline.c */

void
    modem_baudrate(int baud),
    modem_flush(int fd),
    modem_debug(int fd),
	bug_report(char *bbs, char *ver, char *file, int line, char *func, char *cmd, char *s),
	bug_report_textline(char *bbs, char *ver, char *file, int line, char *func, char *cmd, 
		struct text_line *tl),
	problem_report(char *bbs, char *ver, char *cmd, char *s),
	problem_report_textline(char *bbs, char *ver, char *cmd, struct text_line *tl),
	kill_trailing_spaces(char *s),
	space_fill(char *out, int len),
	place_string(char *out, char *s, int loc),
	place_number(char *out, int num, int loc),
	place_hex_number(char *out, int num, int loc),
	disp_column_wise(struct text_line **output, struct text_line *headers, struct text_line *list),
	error_report(void (*callback)(char *s), int clear),
	error_print(void),
	error_print_exit(int code),
	error_clear(void),
	uppercase(char *s),						/* common.c */
	lowercase(char *s),						/* common.c */
	textline_append(struct text_line **tl, char *s),	/* textline.c */
	textline_prepend(struct text_line **tl, char *s),	/* textline.c */
	textline_sort(struct text_line **tl, int position),
	test_host(char *host);					/* daemon.c */

int
	talk(char *str),
	textline_count(struct text_line *tl),
	textline_maxlength(struct text_line *tl),
	spool_fclose(FILE *fp),					/* spool.c */
	spool_abort(FILE *fp),					/* spool.c */
	stricmp(char *s1, char *s2),			/* common.c */
	read_sola(int *bv, int *iv, int *ov),	/* sola.c */
	dectalk(char *str),						/* dectalk.c */
	open_tty(struct tty *t),				/* tty.c */
	close_tty(int fd);						/* tty.c */

long
	get_hexnum(char **str),					/* common.c */
	get_number(char **str);					/* common.c */

time_t
	get_time_t(char **str);					/* common.c */

int
	get_time_interval(char **str, int default_unit, int parse_two_words,
		long *result);

char
    *modem_read(int fd, int timeout),
	*copy_string(char *s),					/* common.c */
	*get_call(char **str),					/* common.c */
	*get_string(char **str),				/* common.c */
	*get_string_to(char **str, char term),	/* common.c */
	*get_word(char **str);					/* common.c */

FILE
	*spool_fopen(char *fn);					/* spool.c */

int
    modem_close(int fd),
    modem_open(char *device),
    modem_dial(char *device, char *phone_number, char *init),
    modem_blind_dial(char *device, char *phone_number, char *init),
    modem_write(int fd, char *s),
	socket_write(int fd, char *buf),
    socket_raw_write(int fd, const char *buf),
    socket_raw_write_n(int fd, const char *buf, size_t n),
	socket_read_raw_line(int fd, char *line, int len, int timeout),
	socket_read_line(int fd, char *line, int len, int timeout),
	socket_accept(int sock),
	socket_accept_nonblock_unmanaged(int sock),
	socket_open(char *host, int port),
	socket_listen(const char *bind_addr, int *port),
	socket_read_pending(int fd),
	socket_close(int fd),
	socket_parse_bindspec(const char *spec, char *buf, size_t bfsz,
		int *port, char **host),
	IsPrintable(char *s);

extern short
	sum_string(char *s);

extern void
	*mem_calloc(int cnt, int size);

void safegets(char *buf, size_t sz);

int
#ifndef SABER
	error_log(char *fmt, ...);
#else
	error_log();
#endif
