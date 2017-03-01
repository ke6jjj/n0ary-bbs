#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "config.h"
#include "bbslib.h"
#include "tools.h"

FILE *fp_city, *fp_first, *fp_last, *fp_zip;

static int
	parse_call(struct callbook_entry *cb, char *s, int len),
	parse_lname(struct callbook_entry *cb, char *s, int len),
	parse_fname(struct callbook_entry *cb, char *s, int len),
	parse_mname(struct callbook_entry *cb, char *s, int len),
	parse_addr(struct callbook_entry *cb, char *s, int len),
	parse_city(struct callbook_entry *cb, char *s, int len),
	parse_state(struct callbook_entry *cb, char *s, int len),
	parse_zip(struct callbook_entry *cb, char *s, int len),
	parse_birth(struct callbook_entry *cb, char *s, int len),
	parse_expire(struct callbook_entry *cb, char *s, int len),
	parse_class(struct callbook_entry *cb, char *s, int len);
static void create_output_files(char *s);
static int append_index(struct callbook_entry *cb, off_t loc);
static void create_callbook(FILE *fp, char *dir);

struct input_record {
	int pos, len;
	int (*func)();
} field[] = {
	{ 0,	6,	parse_call },
	{ 6,	20,	parse_lname },
	{ 30,	11, parse_fname },
	{ 41,	1,	parse_mname },
	{ 42,	35,	parse_addr },
	{ 77,	20,	parse_city },
	{ 97,	2,	parse_state },
	{ 99,	5,	parse_zip },
	{ 114,	2,	parse_birth },
	{ 116,	5,	parse_expire },
	{ 126,	1,	parse_class },
	{ 0,0,NULL }};

#ifdef TEST
int count[10][26];
#endif

static void
create_output_files(char *s)
{
	FILE *fp;
	char area, suffix;
	char fn[256];

	sprintf(fn, "%s/city.indx", s);
	if((fp_city = fopen(fn, "w")) == NULL) {
		printf("could not open %s\n", fn);
		exit(2);
	}

	sprintf(fn, "%s/firstname.indx", s);
	if((fp_first = fopen(fn, "w")) == NULL) {
		printf("could not open %s\n", fn);
		exit(2);
	}

	sprintf(fn, "%s/lastname.indx", s);
	if((fp_last = fopen(fn, "w")) == NULL) {
		printf("could not open %s\n", fn);
		exit(2);
	}

	sprintf(fn, "%s/zip.indx", s);
	if((fp_zip = fopen(fn, "w")) == NULL) {
		printf("could not open %s\n", fn);
		exit(2);
	}

	for(area='0'; area<='9'; area++) {
		char dir[256];
		sprintf(dir, "%s/%c", s, area);
		mkdir(dir, 0777);
		for(suffix='A'; suffix<='Z'; suffix++) {
			sprintf(fn, "%s/%c", dir, suffix);
			if((fp = fopen(fn, "w")) == NULL) {
				printf("could not open %s\n", fn);
				exit(2);
			}
			fclose(fp);
#ifdef TEST
			count[area-'0'][suffix-'A'] = 0;
#endif
		}
	}
}

int
main(int argc, char *argv[])
{
	char callbook[80];
	char outdir[80];
	char fn[80];
	FILE *fp, *tfp;

	if(argc < 2) {
		printf("Enter location of hamcall.129: ");
		safegets(callbook, sizeof(callbook));
		printf("Enter directory for output: ");
		safegets(outdir, sizeof(outdir));
	} else {
		strncpy(callbook, argv[1], sizeof(callbook));
		strncpy(outdir, argv[2], sizeof(outdir));
	}

	strcat(callbook, "/hamcall.129");
	if((fp = fopen(callbook, "r")) == NULL) {
		printf("\n** Could not open callbook file: %s\n", callbook);
		return 1;
	}


	sprintf(fn, "%s/tmp", outdir);
	if((tfp = fopen(fn, "w")) == NULL) {
		printf("\n** Error writing to the directory: %s\n", outdir);
		return 1;
	}
	fclose(tfp);
	unlink(fn);

	create_output_files(outdir);
	create_callbook(fp, outdir);
	fclose(fp);
	fclose(fp_last);
	fclose(fp_first);
	fclose(fp_zip);
	fclose(fp_city);
	return 0;
}

static int
append_index(struct callbook_entry *cb, off_t loc)
{
	struct callbook_index indx;

	if (loc > 0xFFFFFFFFUL)
		/* Can't represent this position in the index file */
		return -1;

	/* Store position in big-endian fashion */
	indx.loc_xdr[0] = (uint8_t)((loc >> 24) & 0xff);
	indx.loc_xdr[1] = (uint8_t)((loc >> 16) & 0xff);
	indx.loc_xdr[2] = (uint8_t)((loc >>  8) & 0xff);
	indx.loc_xdr[3] = (uint8_t)( loc        & 0xff);

	indx.area = cb->callarea[0];
	indx.suffix = cb->suffix[0];

	strncpy(indx.key, cb->lname, 7);
	indx.key[7] = 0;
	fwrite(&indx, sizeof(indx), 1, fp_last);

	strncpy(indx.key, cb->fname, 7);
	indx.key[7] = 0;
	fwrite(&indx, sizeof(indx), 1, fp_first);

	strncpy(indx.key, cb->city, 7);
	indx.key[7] = 0;
	fwrite(&indx, sizeof(indx), 1, fp_city);

	strncpy(indx.key, cb->zip, 7);
	indx.key[7] = 0;
	fwrite(&indx, sizeof(indx), 1, fp_zip);

	return 0;
}

static void
create_callbook(FILE *fp, char *dir)
{
	char buf[256];
	struct callbook_entry cb;
	char suffix = 0, area = 0;
	int cnt = 0;
	FILE *fpw;

	while(fgets(buf, 256, fp)) {
		struct input_record *f = &field[0];
		bzero(&cb, sizeof(cb));
		uppercase(buf);

		while(f->func) {
			if(f->func(&cb, &buf[f->pos], f->len))
				break;
			f++;
		}
#ifdef TEST
		if(count[cb.callarea[0]-'0'][cb.suffix[0]-'A'] < TEST) {
			if(++count[cb.callarea[0]-'0'][cb.suffix[0]-'A'] == TEST)
				cnt++;
#endif
		sprintf(buf, "%s/%c/%c", dir, cb.callarea[0], cb.suffix[0]);
		if((fpw = fopen(buf, "a")) == NULL) {
			printf("Error opening %s\n", buf);
			exit(3);
		}
		if(cb.callarea[0] != area) {
			printf("\n%c: ", cb.callarea[0]);
			area = cb.callarea[0];
		}
		if(cb.suffix[0] != suffix) {
			printf("%c", cb.suffix[0]);
			suffix = cb.suffix[0];
			fflush(stdout);
		}

		if(cb.fname[0] != '>') {
			if (append_index(&cb, ftell(fpw)) != 0) {
				printf("Database too big for index format (wow).\n");
				exit(4);
			}
		}

		if(fwrite(&cb, sizeof(cb), 1, fpw) == 0) {
			printf("Error writing to %s\n", buf);
			exit(4);
		}
		fclose(fpw);

#ifdef TEST
		}
		if(cnt == 260)
			break;
#endif
	}
}

static int
parse_call(struct callbook_entry *cb, char *s, int len)
{
	char buf[80];
	char *q, *p = cb->prefix;

	strncpy(buf, s, len);
	buf[len] = 0;

	q = buf;
	NextChar(q);

	while(*q && isalpha(*q))
		*p++ = *q++;
	
	if(*q == 0) {
		printf("Unexpected NULL in prefix\n");
		exit(10);
	}

	cb->callarea[0] = *q++;

	if(*q == 0) {
		printf("Unexpected NULL in suffix\n");
		exit(10);
	}

	p = cb->suffix;
	while(*q && isalpha(*q))
		*p++ = *q++;
	return OK;
}

static int
parse_lname(struct callbook_entry *cb, char *s, int len)
{
	strncpy(cb->lname, s, len);
	cb->lname[len] = 0;
	kill_trailing_spaces(cb->lname);
	return OK;
}

static int
parse_fname(struct callbook_entry *cb, char *s, int len)
{
	if(*s == ' ') {
		char *p, buf[80];
		strncpy(buf, s, len);
		buf[len] = 0;
		kill_trailing_spaces(buf);
		p = &buf[5];
		NextChar(p);
		sprintf(cb->fname, ">> %s", p);
		cb->lname[0] = 0;
		return ERROR;
	}
	strncpy(cb->fname, s, len);
	cb->fname[len] = 0;
	kill_trailing_spaces(cb->fname);
	return OK;
}

static int
parse_mname(struct callbook_entry *cb, char *s, int len)
{
	cb->mname[0] = *s;
	return OK;
}

static int
parse_addr(struct callbook_entry *cb, char *s, int len)
{
	strncpy(cb->addr, s, len);
	cb->addr[len] = 0;
	kill_trailing_spaces(cb->addr);
	return OK;
}

static int
parse_city(struct callbook_entry *cb, char *s, int len)
{
	strncpy(cb->city, s, len);
	cb->city[len] = 0;
	kill_trailing_spaces(cb->city);
	return OK;
}

static int
parse_state(struct callbook_entry *cb, char *s, int len)
{
	cb->state[0] = *s++;
	cb->state[1] = *s;
	return OK;
}

static int
parse_zip(struct callbook_entry *cb, char *s, int len)
{
	strncpy(cb->zip, s, len);
	cb->zip[len] = 0;
	return OK;
}

static int
parse_birth(struct callbook_entry *cb, char *s, int len)
{
	cb->birth[0] = *s++;
	cb->birth[1] = *s;
	return OK;
}

static int
parse_expire(struct callbook_entry *cb, char *s, int len)
{
	strncpy(cb->exp, s, len);
	cb->exp[len] = 0;
	return OK;
}

static int
parse_class(struct callbook_entry *cb, char *s, int len)
{
	cb->class[0] = *s;
	return OK;
}

