#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "version.h"
#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"

static struct PhoneDefinition *PH = NULL;
static struct PortDefinition *PD = NULL;
static struct TncDefinition *TD = NULL;

char *
build_sid(void)
{
	char *s = BBS_SID;
	char *v = BBS_VERSION;
	static char sid[80];
	char *p = sid;

	while(*s) {
		if(*s == '%') {
			while(*v)
				*p++ = *v++;
			s++;
			continue;
		}
		*p++ = *s++;
	}
	*p = 0;
	return sid;
}

char *
daemon_version(char *name, char *call)
{
	static char ver[40];
	sprintf(ver, "[%s-%s-%s]", name, call, BBS_VERSION);
	return ver;
}

int
get_daemon_version(char *id)
{
	char *p = (char*)rindex(id, '-');
	int rev = 0;

	if(p == NULL)
		return ERROR;

	p++;

	/* version numbers should look like this AA.BB.CC
	 * this should be turned into this AABBCC.
	 */

	while(*p) {
		rev = (rev * 100) + get_number(&p);
		if(*p != '.')
			return rev;
		p++;
	}
	return ERROR;
}

	
static void
build_ports(char *s)
{
	char *c, *alias;
	int i, len;
	struct PortDefinition
		**tmp = &PD,
		*pd = malloc_struct(PortDefinition);

	pd->name = copy_string(get_string(&s));

	c = get_string(&s);
	uppercase(c);
	if(!strcmp(c, "TNC"))
		pd->type = tTNC;
	else if(!strcmp(c, "PHONE"))
		pd->type = tPHONE;
	else if(!strcmp(c, "CONSOLE"))
		pd->type = tCONSOLE;
	else if(!strcmp(c, "SYSTEM"))
		pd->type = FALSE;
	else if(!strcmp(c, "TCP"))
		pd->type = tTCP;
	else if(!strcmp(c, "SMTP"))
		pd->type = tSMTP;

	c = get_string(&s);
	uppercase(c);
	if(!strcmp(c, "TRUE"))
		pd->secure = TRUE;

	c = get_string(&s);
	uppercase(c);
	if(!strcmp(c, "TRUE"))
		pd->show = TRUE;

		/* alias should not be more than 8 characters. We rely on this
		 * so that we can center the fields.
		 */

	alias = copy_string(get_string(&s));
	len = strlen(alias);

	for(i=0; i<8; i++) pd->alias[i] = ' ';
	i = (8 - len)/2;
	strncpy(&(pd->alias[i]), alias, len);
	free(alias);

	while(*tmp != NULL)
		tmp = &((*tmp)->next);
	*tmp = pd;
}

static void
build_tncs(char *s)
{
	int optional_flags;
	struct TncDefinition
		**tmp = &TD,
		*td = malloc_struct(TncDefinition);

	td->name = copy_string(get_string(&s));
	td->port = get_number(&s);
	td->monitor = get_number(&s);
	td->device = copy_string(get_string(&s));
	td->host = copy_string(get_string(&s));

	td->ax25.t1 = get_number(&s);
	td->ax25.t2 = get_number(&s);
	td->ax25.t3 = get_number(&s);
	td->ax25.maxframe = get_number(&s);
	td->ax25.paclen = get_number(&s);
	td->ax25.n2 = get_number(&s);
	td->ax25.flags = 0;

	/* Get SLIP encoding flags, if present. */
	optional_flags = get_number(&s);
	if (optional_flags != ERROR)
		td->ax25.flags = optional_flags;

	while(*tmp != NULL)
		tmp = &((*tmp)->next);
	*tmp = td;
}

static void
build_phones(char *s)
{
	struct PhoneDefinition
		**tmp = &PH,
		*ph = malloc_struct(PhoneDefinition);

	ph->name = copy_string(get_string(&s));
	ph->device = copy_string(get_string(&s));
	ph->init_str = copy_string(s);

	while(*tmp != NULL)
		tmp = &((*tmp)->next);
	*tmp = ph;
}

struct PortDefinition *
port_table(void)
{
	if(PD == NULL)
		bbsd_get_variable_list("PORT", build_ports);
	return PD;
}
		
struct PortDefinition *
port_find(char *name)
{
	struct PortDefinition *pd;

	if(PD == NULL)
		bbsd_get_variable_list("PORT", build_ports);
	pd = PD;

	while(pd) {
		if(!stricmp(name, pd->name))
			return pd;
		NEXT(pd);
	}
	return NULL;
}

struct TncDefinition *
tnc_table(void)
{
	if(TD == NULL)
		bbsd_get_variable_list("TNC", build_tncs);
	return TD;
}
		
struct TncDefinition *
tnc_find(char *name)
{
	struct TncDefinition *td;

	if(TD == NULL)
		bbsd_get_variable_list("TNC", build_tncs);
	td = TD;

	while(td) {
		if(!stricmp(name, td->name))
			return td;
		NEXT(td);
	}
	return NULL;
}

struct PhoneDefinition *
phone_table(void)
{
	if(PH == NULL)
		bbsd_get_variable_list("PHONE", build_phones);
	return PH;
}
		
struct PhoneDefinition *
phone_find(char *name)
{
	struct PhoneDefinition *pd;

	if(PH == NULL)
		bbsd_get_variable_list("PHONE", build_phones);
	pd = PH;

	while(pd) {
		if(!stricmp(name, pd->name))
			return pd;
		NEXT(pd);
	}
	return NULL;
}

char *
port_name(int indx)
{
	struct PortDefinition *pd;
	if(PD == NULL)
		bbsd_get_variable_list("PORT", build_ports);
	pd = PD;

	while(pd && indx--)
		NEXT(pd);

	if(pd == NULL)
		return NULL;
	return pd->name;
}

int
port_indx(char *name)
{
	int i = 0;
	struct PortDefinition *pd;
	if(PD == NULL)
		bbsd_get_variable_list("PORT", build_ports);
	pd = PD;

	while(pd && stricmp(pd->name, name)) {
		NEXT(pd);
		i++;
	}

	if(pd == NULL)
		return ERROR;

	return i;
}

int
port_secure(char *name)
{
	struct PortDefinition *t = port_find(name);
	return t->secure;
}

int
port_type(char *name)
{
	struct PortDefinition *t = port_find(name);
	return t->type;
}

char *
port_alias(char *name)
{
	struct PortDefinition *t = port_find(name);
	return t->alias;
}

int
port_show(char *name)
{
	struct PortDefinition *t = port_find(name);
	return t->show;
}

int
tnc_port(char *name)
{
	struct TncDefinition *t = tnc_find(name);
	return t->port;
}

int
tnc_monitor(char *name)
{
	struct TncDefinition *t = tnc_find(name);
	return t->monitor;
}

char *
tnc_device(char *name)
{
	struct TncDefinition *t = tnc_find(name);
	return t->device;
}

char *
tnc_host(char *name)
{
	struct TncDefinition *t = tnc_find(name);
	return t->host;
}

struct Tnc_ax25 *
tnc_ax25(char *name)
{
	struct TncDefinition *t = tnc_find(name);
	return &(t->ax25);
}

char *
phone_device(char *name)
{
	struct PhoneDefinition *t = phone_find(name);
	return t->device;
}

char *
phone_init(char *name)
{
	struct PhoneDefinition *t = phone_find(name);
	return t->init_str;
}

time_t
str2time_t(char *s)
{
	time_t t = time(NULL);
	struct tm ltm, *tm = localtime_r(&t, &ltm);
	time_t dt = 0;

	strptime(s, "%y%m%d/%H%M", tm);
	tm->tm_sec = 0;
#ifdef SUNOS
	dt = timelocal(tm);
#else
	dt = mktime(tm);
#endif
	return dt;
}

int
parse_callsign(char *call)
{
	char *s = call;
	int digit = 0;
	int prefix = 0;
	int suffix = 0;
	int *letter = &prefix;

		/* first see if it is a name */
	while(*s) {
			/* look for bad characters */
		if(!isalnum(*s))
			return CALLisSUSPECT;

		if(!isalpha(*s))
			break;
		s++;
	}

	if(*s == 0)
		return CALLisSUSPECT;

	s = call;
	if(strlen(s) < 4)
		return CALLisSUSPECT;

	if(isdigit(*s))
		s++;

	while(*s) {
		if(isalpha(*s))
			(*letter)++;
		if(isdigit(*s)) {
			digit++;
			letter = &suffix;
		}
		s++;
	}
	if(prefix == 0 || digit != 1 || suffix == 0)
		return CALLisSUSPECT;

	return CALLisOK;
}

void
show_configuration_rules(char *fn)
{
	FILE *fp = stdout;
	int file = FALSE;

	if(fn != NULL) {
		if((fp = fopen(fn, "a")) == NULL)
			return;
		file = TRUE;
	}

fprintf(fp, "####\n");
fprintf(fp, "##  BBS Configuration Rules\n");
fprintf(fp, "####\n");
fprintf(fp, "#\n");
fprintf(fp, "# Entries in this file are keyword value pairs. The keyword is\n");
fprintf(fp, "# a string that starts in column 1 and begins with a letter.\n");
fprintf(fp, "# The value field is separted from the keyword by white space.\n");
fprintf(fp, "#\n");
fprintf(fp, "# Values have an associated type. The posible types are:\n");
fprintf(fp, "#   STRING, NUMBER, DIRECTORY, FILE and TIME\n");
fprintf(fp, "#\n");
fprintf(fp, "# When specifying a DIRECTORY or FILE if the first character\n");
fprintf(fp, "# is a slash '/' then the path is considered to be absolute.\n");
fprintf(fp, "# If it begins with a character other than a slash the field\n");
fprintf(fp, "# represented by BBS_DIR is prepended. DIRECTORY values should\n");
fprintf(fp, "# not end with a slash.\n");
fprintf(fp, "#\n");
fprintf(fp, "# The TIME values are a number followed by the quantity\n");
fprintf(fp, "# definition, ie. SECONDS, MINUTES, HOURS, DAYS, WEEKS,\n");
fprintf(fp, "# MONTHS or YEARS.\n");
fprintf(fp, "#\n");
fprintf(fp, "# Comments are lines that begin with a '#'. Comments cannot\n");
fprintf(fp, "# be appended to the end of KEYWORD/VALUE pairs.\n");
fprintf(fp, "#\n");
fprintf(fp, "#\n");

	if(file)
		fclose(fp);
}

void
show_reqd_configuration(struct ConfigurationList *cl, char *proc_name, char *fn)
{
	FILE *fp = stdout;
	int file = FALSE;

	if(fn != NULL) {
		if((fp = fopen(fn, "a")) == NULL)
			return;
		file = TRUE;
	}

	fprintf(fp, "\n####\n");
	fprintf(fp, "##  %s\n", proc_name);
	fprintf(fp, "####\n");

	while(cl->token != NULL) {
		switch(cl->type) {
		case tCOMMENT:
			fprintf(fp, "# %s\n", cl->token);
			break;
		case tTIME:
			fprintf(fp, "%s\t[time]\n",
				cl->token);
			break;
		case tINT:
			fprintf(fp, "%s\t[number]\n", cl->token);
			break;
		case tDIRECTORY:
			fprintf(fp, "%s\t[directory_path]\n", cl->token);
			break;
		case tSTRING:
			fprintf(fp, "%s\t[string]\n", cl->token);
			break;
		case tFILE:
			fprintf(fp, "%s\t[path_filename]\n", cl->token);
			break;
		default:
			fprintf(fp, "%s\t[UNKNOWN]\n", cl->token);
		}
		cl++;
	}

	if(file)
		fclose(fp);
}


