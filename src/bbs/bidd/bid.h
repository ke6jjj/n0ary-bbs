#include <time.h>

#define Error(s)	"NO, "s"\n"
#define Ok(s)		"OK, "s"\n"

struct bid_entry {
	struct bid_entry *next;
	time_t	seen;
	char	str[LenBID];
};

extern int
	dbug_level,
	bid_image;
#define CLEAN	0
#define DIRTY	1

extern time_t
	time_now,
	Bidd_Age;

extern int
	Bidd_Port;

extern char
	*Bbs_Host,
	*Bidd_File;

extern int
	read_file(char *filename),
	read_new_file(char *filename),
	hash_delete_bid(char *s),
	hash_write(FILE *fp);

extern struct bid_entry
	*hash_get_bid(char *s),
	*hash_create_bid(char *s);

extern void
    age(void),
	hash_init(void),
	hash_stats(void),
	uppercase(char *s);

extern time_t
	Time(time_t *t),
	date2time(char *s);

extern char
	*get_string(char **str),
	*write_file(void),
	*parse(char *s),
	*hash_cnt(void),
	*add_bid(char *bid),
	*chk_bid(char *bid),
	*chk_mid(char *bid),
	*delete_bid(char *s),
	*time2date(time_t t);
