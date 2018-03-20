#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "tokens.h"
#include "callbk.h"
#include "message.h"
#include "function.h"
#include "user.h"
#include "server.h"
#include "vars.h"
#include "search.h"
#include "bbscommon.h"
#include "parse.h"

#define cbDONE		0
#define cbLNAME		1
#define cbFNAME		2
#define cbCITY		4
#define cbSTATE		8
#define cbZIP		0x10

#define cbPREFIX	1
#define cbCALLAREA	2
#define cbSUFFIX	3

#define CB_PRINT(p) {\
	if(cbtl) {\
		textline_append(cbtl, p);\
	} else {\
		PRINTF("%s\n", p);\
	}}

static struct text_line
	**cbtl = NULL;

static char *
	callbk_date_cnvrt(char *str);

static int
	prompt_for_patterns(void),
	patterns_from_tokenlist(struct TOKEN *t),
	search_callbook(int server),
	parse_lookup_request(struct TOKEN *t),
	check_print(char *prefix),
	lookup_area(int num, char *prefix, char *suffix),
	callbk_mode,
	terse,
	match_on,
	ask_for(int mask, char *str, char *result, int cnt);

static void
	parse_for_lookup(struct msg_dir_entry *m),
	parse_for_search(struct msg_dir_entry *m),
	display_qth(char *prefix, char *area, char *suffix),
	display_single_line_terse(FILE *fptmp),
	merge_strings(char *to, char *from);

static struct callbook_entry
	entry, match;

static struct callbook_requests {
	struct callbook_requests *next;
	char	prefix[10];
	char	callarea[5];
	char	suffix[10];
	char	match[10];
} *last, *CB_Req = NULL;

static int
log_request(char *prefix, char *callarea, char *suffix)
{
	struct callbook_requests *cb;
	int match = FALSE;

	if((cb = malloc_struct(callbook_requests)) == NULL)
		return ERROR;

	if(CB_Req)
		last->next = cb;
	else
		CB_Req = cb;
	cb->next = NULL;
	last = cb;

	if(*prefix) {
		case_strcpy(cb->prefix, prefix, AllUpperCase);
		strcpy(cb->match, cb->prefix);
	} else {
		strcpy(cb->match, "?");
		match = TRUE;
	}

	if(*callarea) {
		strcpy(cb->callarea, callarea);
		strcat(cb->match, cb->callarea);
	} else {
		strcat(cb->match, "?");
		match = TRUE;
	}

	case_strcpy(cb->suffix, suffix, AllUpperCase);
	if(match)
		strcat(cb->match, suffix);
	else
		cb->match[0] = 0;
	return OK;
}
int
callbk(void)
{
	struct TOKEN *t = TokenList;
	int token = t->token;

	NEXT(t);
	switch(token) {
	case SEARCH:
		if(t->token == END) {
			if(prompt_for_patterns())
				return OK;
		} else {
			if(patterns_from_tokenlist(t))
				return OK;
		}
		search_callbook(FALSE);
		break;

	case LOOKUP:
		return parse_lookup_request(t);

	default:
		return error(1);
	}
	return OK;
}

static void
free_callbook_requests(void)
{
	struct callbook_requests *cb;
	while(CB_Req) {
		cb = CB_Req;
		NEXT(CB_Req);
		free(cb);
	}
}

int
lookup_call_callbk(char *call, struct callbook_entry *cb)
{
	char prefix[3], suffix[4], *p = call;
	int callarea, i = 0;
	FILE *fp;
	char name[40];

	while(!isdigit(*p)) {
		if(!*p)
			return ERROR;
		prefix[i++] = *p++;
	}
	prefix[i] = 0;

	callarea = atoi(p++);
	strcpy(suffix, p);

	sprintf(name, "%s/%d/%c", Bbs_Callbk_Path, callarea, suffix[0]);
	if((fp = fopen(name, "r")) == NULL) {
		PRINTF("Callbook unavailable [%s]\n", name);
		return ERROR;
	}

	while(1) {
		int result;

		if(!fread(cb, SizeofCALLBK, 1, fp))
			break;

		result = strcmp(cb->suffix, suffix);
		if(result == 0)
			if(!strcmp(cb->prefix, prefix)) {
				fclose(fp);
				return OK;
		}
	}
	fclose(fp);
	return ERROR;
}

static int
ask_for(int mask, char *str, char *result, int cnt)
{
	char buf[80];

	PRINT(str);
	if(NeedsNewline)
		PRINT("\n");
	if (GETS(buf, 80) == NULL)
		return -1;

	buf[cnt] = 0;
	strcpy(result, buf);

	if(*result) {
		case_string(result, AllUpperCase);
		match_on |= mask;
	}

	return 0;
}

static int
parse_lookup_request(struct TOKEN *t)
{
	char prefix[10], suffix[10], callarea[2];
	int where = cbPREFIX;
	
	callarea[1] = 0;
	while(TRUE) {
		switch(t->token) {
		case NUMBER:
			switch(where) {
			case cbPREFIX:
				prefix[0] = 0;
				where = cbSUFFIX;
				break;
			case cbCALLAREA:
				if(t->value > 9)
					return bad_cmd(15, t->location);
				callarea[0] = t->lexem[0];
				where = cbSUFFIX;
				break;
			case cbSUFFIX:
				return bad_cmd(16, t->location);
			}
			break;

		case WORD:
			switch(where) {
			case cbPREFIX:
				if(isCall(t->lexem)) {
					char *p = t->lexem;
					while(isalpha(*p)) p++;
					callarea[0] = *p;
					*p++ = 0;
					strcpy(prefix, t->lexem);
					strcpy(suffix, p);
					log_request(prefix, callarea, suffix);
					break;
				}
						
				case_strcpy(prefix, t->lexem, AllUpperCase);
				where = cbCALLAREA;
				break;
			case cbCALLAREA:
				if(isalpha(t->lexem[0]))
					return bad_cmd(17, t->location);
				callarea[0] = t->lexem[0];
				case_strcpy(suffix, &(t->lexem[1]), AllUpperCase);
				log_request(prefix, callarea, suffix);
				where = cbPREFIX;
				break;

			case cbSUFFIX:
				case_strcpy(suffix, t->lexem, AllUpperCase);
				log_request(prefix, callarea, suffix);
				where = cbPREFIX;
				break;
			}
			break;

		case COMMA:
			break;

		case UNKNOWN:
			if(t->lexem[0] != '?')
				return bad_cmd(14, t->location);

			switch(where) {
			case cbPREFIX:
				prefix[0] = 0;
				where = cbCALLAREA;
				break;
			case cbCALLAREA:
				callarea[0] = 0;
				where = cbSUFFIX;
				break;
			case cbSUFFIX:
				return bad_cmd(18, t->location);
			}
			break;

		case END:
			switch(where) {
			case cbPREFIX:
				last = CB_Req;
				while(last) {
					if(last->match[0]) {
						char str[20];
						sprintf(str, "Searching for matches to %s ======",
							last->match);
						CB_PRINT(str);
					}

					display_qth(last->prefix, last->callarea, last->suffix);
					if(last->match[0])
						CB_PRINT("");
					NEXT(last);
				}
				free_callbook_requests();
				return OK;

			case cbSUFFIX:
			case cbCALLAREA:
				return bad_cmd(20, t->location);
			}

		default:
			return bad_cmd(16, t->location);
		}
		NEXT(t);
	}
}


static int
patterns_from_tokenlist(struct TOKEN *t)
{
	int which = cbLNAME;
	char *str = match.lname;
	int len = 25;
	match_on = 0;

	match.lname[0] = 0;
	match.fname[0] = 0;
	match.city[0] = 0;
	match.state[0] = 0;

	while(TRUE) {
		switch(t->token) {
		case STRING:
		case NUMBER:
		case WORD:
			match_on |= which;
			strncpy(str, t->lexem, len);
			case_string(str, AllUpperCase);
			NEXT(t);
			break;
			
		case COMMA:
			switch(which) {
			case cbLNAME:
				which = cbFNAME;
				len = 12;
				str = match.fname;
				break;
			case cbFNAME:
				which = cbCITY;
				len = 21;
				str = match.city;
				break;
			case cbCITY:
				which = cbSTATE;
				len = 3;
				str = match.state;
				break;
			case cbZIP:
				which = cbZIP;
				len = 6;
				str = match.zip;
				break;
			}
			NEXT(t);
			break;

		case END:
			if(!(match_on & cbLNAME))
				return error(10);
			return OK;

		case UNKNOWN:
		default:
			bad_cmd(14, t->location);
			return ERROR;
		}
	}
}

static int
prompt_for_patterns(void)
{
	match_on = 0;
	if (ask_for(cbLNAME, " Last name: ", match.lname, 24))
		return ERROR;
	if(!(match_on & cbLNAME) && !ImSysop)
		return error(10);
	if (ask_for(cbFNAME, "First name: ", match.fname, 11))
		return ERROR;
	if (ask_for(cbCITY, "      City: ", match.city, 20))
		return ERROR;
	if (ask_for(cbSTATE, "     State: ", match.state, 2))
		return ERROR;
	if (ask_for(cbZIP, "       Zip: ", match.zip, 5))
		return ERROR;
	return OK;
}

static void
display_single_line_terse(FILE *fptmp)
{
	int i;
	char buf[256], tmp[80];
	char *p;

	for(i=0; i<256; i++)
		buf[i] = ' ';

	sprintf(tmp, "%2s%s%-3s", entry.prefix, entry.callarea, entry.suffix);
	merge_strings(buf, tmp);

	if(entry.mname[0] != ' ' && entry.mname[0])
		sprintf(tmp, "%s %s, %s %s", 
			entry.class, entry.lname, entry.fname, entry.mname);
	else
		sprintf(tmp, "%s %s, %s", entry.class, entry.lname, entry.fname);
	merge_strings(&buf[7], tmp);

	p = &entry.addr[strlen(entry.addr) - 1];
	while(*p == ' ')
		*p-- = 0;
	sprintf(&buf[33], "%s, %s, %s %s", 
		entry.addr, entry.city, entry.state, entry.zip);

	fprintf(fptmp, "%s\n", buf);
}

static void
merge_strings(char *to, char *from)
{
	while(*from)
		*to++ = *from++;
}

static int
check_call_for_match(FILE *fptmp, char num, char suffix, int loc)
{
	FILE *fp;
	char name[40];

	sprintf(name, "%s/%c/%c", Bbs_Callbk_Path, num, suffix);
	if((fp = fopen(name, "r")) == NULL)
		return 0;

	fseek(fp, loc, 0);
	if(!fread(&entry, SizeofCALLBK, 1, fp)) {
		fclose(fp);
		return 0;
	}
	fclose(fp);

	if(match_on & cbFNAME) {
		case_string(entry.fname, AllUpperCase);
		if(strncmp(entry.fname, match.fname, strlen(match.fname)))
			return 0;
	}

	if(match_on & cbLNAME) {
		case_string(entry.lname, AllUpperCase);
		if(strncmp(entry.lname, match.lname, strlen(match.lname)))
			return 0;
	}

	if(match_on & cbCITY) {
		case_string(entry.city, AllUpperCase);
		if(strncmp(entry.city, match.city, strlen(match.city)))
			return 0;
	}

	if(match_on & cbSTATE)
		if(strncmp(entry.state, match.state, strlen(match.state)))
			return 0;

	if(match_on & cbZIP)
		if(strncmp(entry.zip, match.zip, 5))
			return 0;

	display_single_line_terse(fptmp);
	return 1;
}

static int
search_callbook(int server)
{
	int cnt = 0;
	FILE *fptmp, *fpindx;
	char filename[80];
	int len;
	char *p, buf[256], pattern[20];
	struct callbook_index cbi;

	if(match_on & cbLNAME) {
		sprintf(filename, "%s/lastname.indx", Bbs_Callbk_Path);
		p = match.lname;
	} else
		if(match_on & cbCITY) {
			sprintf(filename, "%s/city.indx", Bbs_Callbk_Path);
			p = match.city;
		} else {
			if(match_on & cbZIP) {
				sprintf(filename, "%s/zip.indx", Bbs_Callbk_Path);
				p = match.zip;
			} else {
				PRINT("Must supply either a LNAME, CITY or ZIP\n");
				return ERROR;
			}
		}

	if((fpindx = fopen(filename, "r")) == NULL)
		return ERROR;

	sprintf(filename, "/tmp/callbk.%d", getpid());
	if((fptmp = fopen(filename, "w+")) == NULL) {
		fclose(fpindx);
		return ERROR;
	}

	system_msg(11);

	strcpy(pattern, p);

	len = strlen(pattern);
	if(len > 7) {
		pattern[7] = 0;
		len = 7;
	}

			/* begin by narrowing down the area we need to search */
	{
		int length, loc;

		fseek(fpindx, 0, 2);
		length = (ftell(fpindx) + 1) / SizeofCBINDEX;
		loc = search_start(length);

		while(TRUE) {
			fseek(fpindx, loc*SizeofCBINDEX, 0);
			fread(&cbi, SizeofCBINDEX, 1, fpindx);
			if(search_range() < 10)
				break;
			loc = search_next(strncmp(cbi.key, pattern, len));
		}

		loc = search_end(strncmp(cbi.key, pattern, len));
		fseek(fpindx, SizeofCBINDEX*loc, 0);
	}

	while(fread(&cbi, SizeofCBINDEX, 1, fpindx)) {
		int result = strncmp(cbi.key, pattern, len);

		if(result > 0)
			break;

		/* Unmarshal the record location from its disk format */
		uint32_t loc = 
			(cbi.loc_xdr[0] << 24) +
			(cbi.loc_xdr[1] << 16) +
			(cbi.loc_xdr[2] << 8) +
			(cbi.loc_xdr[3]);

		if(!result)
			if(check_call_for_match(fptmp, cbi.area, cbi.suffix, loc)) {
				cnt++;
				if(!server) 
					if(!(cnt % 20))
						PRINTF("%d matches found so far ...\n", cnt);
			}

		if(!ImSysop && !server)
			if(cnt > 200) {
				system_msg(12);
				fclose(fptmp);
				fclose(fpindx);
				unlink(filename);
				return OK;
			}
	}

	fclose(fpindx);

	if(!server)
		if(check_long_xfer_abort(13, cnt)) {
			fclose(fptmp);
			unlink(filename);
			return OK;
		}

	rewind(fptmp);

	if(!server) {
		init_more();
		more();
		more();
	}

	sprintf(buf, "\n%d calls match request", cnt);
	CB_PRINT(buf);
	CB_PRINT("");

	while(fgets(buf, 256, fptmp)) {
		buf[strlen(buf)-1] = 0;
		CB_PRINT(buf);
		if(!server)
			if(more())
				break;
	}

	fclose(fptmp);
	unlink(filename);
	return OK;
}

static void
display_qth(char *prefix, char *area, char *suffix)
{
	int found = FALSE;

	terse = 0;
	if(*prefix == 0)
		terse = 1;

	if(*area == 0) {
		int i;
		terse = 1;
		for(i=0; i<10; i++)
			found |= lookup_area(i, prefix, suffix);
	} else
		found |= lookup_area(atoi(area), prefix, suffix);

	if(!found) {
		char buf[128];
		sprintf(buf, "Callsign %s%s%s not found in callbook", prefix,area,suffix);
		CB_PRINT(buf);
		if(!terse)
			CB_PRINT("");
	}
}

static int
lookup_area(int num, char *prefix, char *suffix)
{
	FILE *fp;
	int found = 0;
	char name[40];

	sprintf(name, "%s/%d/%c", Bbs_Callbk_Path, num, *suffix);
	if((fp = fopen(name, "r")) == NULL) 
		return 0;

	while(1) {
		int result;

		if(!fread(&entry, SizeofCALLBK, 1, fp))
			break;

		result = strcmp(entry.suffix, suffix);
		if(result == 0)
			found |= check_print(prefix);
	}
	fclose(fp);
	return found;
}

static char *
callbk_date_cnvrt(char *str)
{
	static char out[20];
	int da = atoi(&str[2]);
	int mo, yr, year;
	int leap;
	
	static int days[2][11] = {
		{ 31, 27, 31, 30, 31, 30, 31, 31, 30, 31, 30},
		{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30} };

	str[2] = 0;
	yr = atoi(str);

	if(yr < 80)
		year = yr + 2000;
	else
		year = yr + 1990;
	
	leap = (year % 4) ? 0:1; /* DOH! Good enough until 2100 -KE6JJJ */

	for(mo=0; mo<11; mo++) {	
		if(da < days[leap][mo])
			break;
		da -= days[leap][mo];
	}
	sprintf(out, "%2d/%02d/%02d", mo+1, da+1, yr);
	return out;
}

static int
check_print(char *prefix)
{
	char buf[80];

	if(*prefix != 0) {
		if(strcmp(entry.prefix, prefix))
			return 0;
	}

	if(entry.fname[0] == '>') {
		sprintf(buf, "%s%s%s\t%s", 
			entry.prefix, entry.callarea, entry.suffix, entry.fname);
		CB_PRINT(buf);
		if(!terse)
			CB_PRINT("");
		return 1;
	}

	if(terse) {
		sprintf(buf, "%s%s%s\t%s\t%s %s. %s",
			entry.prefix, entry.callarea, entry.suffix, entry.class,
			entry.fname, entry.mname, entry.lname);
		CB_PRINT(buf);
	} else {
		sprintf(buf, "%s%s%s  %s  (%s)   exp: %s",
			entry.prefix, entry.callarea, entry.suffix, entry.class, 
			entry.birth, callbk_date_cnvrt(entry.exp));
		CB_PRINT(buf);
		sprintf(buf, "%s %s. %s", entry.fname, entry.mname, entry.lname);
		CB_PRINT(buf);
		sprintf(buf, "%s", entry.addr);
		CB_PRINT(buf);
		sprintf(buf, "%s, %s %s", entry.city, entry.state, entry.zip);
		CB_PRINT(buf);
		CB_PRINT("");
	}
	return 1;
}

void
callbk_server(int num, char *mode)
{
	struct msg_dir_entry *m;

	case_string(mode, AllUpperCase);

	InServer = 'C';
	strcpy(usercall, "CALLBK");
	ImSysop = TRUE;
	IGavePassword = TRUE;
	MyHelpLevel = 0;

	if(userd_open())
		return;
	if(msgd_open())
		return;

	msg_LoginUser(usercall);
	msg_SysopMode();
	build_full_message_list();

	if((m = msg_locate(num)) == NULL)
		return;

	if(!strcmp(mode, "LOOKUP")) {
		callbk_mode = LOOKUP;
		parse_for_lookup(m);
	} else
		if(!strcmp(mode, "SEARCH")) {
			callbk_mode = SEARCH;
			parse_for_search(m);
		} else
			if(!strcmp(mode, "INFO"))
				callbk_mode = INFO;
			else
				callbk_mode = NONE;

	msg_rply(num);
	msgd_cmd_num(msgd_xlate(mKILL), num);
}
			
static void
parse_for_lookup(struct msg_dir_entry *m)
{
	struct text_line *tl;
	char request[4096];
	int cnt = 0;

	msg_ReadBody(m);
	tl = m->body;

	request[0] = 0;
	while(tl) {
		char *p = tl->s;
		
		while(*p) {
			char *call = get_string(&p);
			int len = strlen(call);
			if(len < 3 || len > 6)
				continue;
			sprintf(request, "%s %s", request, call);
			cnt++;
		}
		NEXT(tl);
	}

	if(cnt)
		parse_simple_string(&request[1], DummyOperands);
	else
		TokenList = NULL;
}

static void
copy_field(char *d, char *s, int len)
{
	char *c = (char*)index(s, '\n');
	if(c)
		*c = 0;

	NextSpace(s);
	NextChar(s);
	if(strlen(s) > len)
		s[len] = 0;

	if(*s)
		strcpy(d, s);
}

/*ARGSUSED*/
static void
parse_for_search(struct msg_dir_entry *m)
{
	struct text_line *tl;
	match_on = 0;

	msg_ReadBody(m);
	tl = m->body;

	while(tl) {
		char *buf = tl->s;
		uppercase(buf);

		if(!strncmp(buf, "LNAME", 5)) {
			copy_field(match.lname, &buf[5], 24);
			match_on |= cbLNAME;
		} else if(!strncmp(buf, "FNAME", 5)) {
			copy_field(match.fname, &buf[5], 11);
			match_on |= cbFNAME;
		} else if(!strncmp(buf, "CITY", 4)) {
			copy_field(match.city, &buf[4], 20);
			match_on |= cbCITY;
		} else if(!strncmp(buf, "STATE", 5)) {
			copy_field(match.state, &buf[5], 2);
			match_on |= cbSTATE;
		} else if(!strncmp(buf, "ZIP", 3)) {
			copy_field(match.zip, &buf[3], 5);
			match_on |= cbZIP;
		}

		NEXT(tl);
	}
}

static int
show_criteria()
{
	char buf[256];

	if(match_on == 0) {
		CB_PRINT("No search criteria specified");
		CB_PRINT("");
		return ERROR;
	}

	if(match_on & cbLNAME) {
		sprintf(buf, "LNAME: %s", match.lname);
		CB_PRINT(buf);
	}
	if(match_on & cbFNAME) {
		sprintf(buf, "FNAME: %s", match.fname);
		CB_PRINT(buf);
	}
	if(match_on & cbCITY) {
		sprintf(buf, "CITY: %s", match.city);
		CB_PRINT(buf);
	}
	if(match_on & cbSTATE) {
		sprintf(buf, "STATE: %s", match.state);
		CB_PRINT(buf);
	}
	if(match_on & cbZIP) {
		sprintf(buf, "ZIP: %s", match.zip);
		CB_PRINT(buf);
	}

	if(!(match_on & cbLNAME) && !(match_on & cbZIP) &&
		!((match_on & cbCITY) && (match_on & cbSTATE))) {

		CB_PRINT("The following are minimum search criteria:");
		CB_PRINT("  LNAME");
		CB_PRINT("  ZIP");
		CB_PRINT("  CITY & STATE");
		CB_PRINT("");
		return ERROR;
	}

	return OK;
}

static void
reply_with_help(void)
{
	FILE *fp;
	char buf[256];
	sprintf(buf, "%s/.SERVER.CALLBK", Bbs_Info_Path);
	if((fp = fopen(buf, "r")) == NULL) {
		CB_PRINT("Error opening information file.. aborting");
		return;
	}
	while(fgets(buf, 255, fp)) {
		buf[strlen(buf)-1] = 0;
		CB_PRINT(buf);
	}
	fclose(fp);
}

void
gen_callbk_body(struct text_line **tl)
{
	char buf[80];

	cbtl = tl;

	switch(callbk_mode) {
	case LOOKUP:
		parse_lookup_request(TokenList);
		break;
	case SEARCH:
		if(show_criteria() == OK)
			search_callbook(TRUE);
		else
			reply_with_help();
		break;

	case INFO:
		reply_with_help();
		break;

	case NONE:
		CB_PRINT("Poorly formed callbook request ...");
		CB_PRINT("Your subject will route your request to the callbook");
		CB_PRINT("server. Possible subjects are:");
		CB_PRINT("");
		CB_PRINT("  LOOKUP");
		CB_PRINT("    Look up a callsign in the callbook");
		CB_PRINT("");
		CB_PRINT("  SEARCH\n");
		CB_PRINT("    Search for a ham by last name and qth");
		CB_PRINT("");
		CB_PRINT("  INFO");
		CB_PRINT("    Reply with information on how to use the callbk server");
	}

	CB_PRINT("");
	sprintf(buf, "73, %s", Bbs_Call);
	CB_PRINT(buf);
}
