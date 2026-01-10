#include <stdio.h>
#include <string.h>
#include <time.h>

#include "c_cmmn.h"
#include "tools.h"
#include "wp.h"

long new_level = WP_Init;
long bbs_mode = FALSE;

char *
set_level(struct active_processes *ap, int token, char *s)
{
	char *result;

	switch(token) {
	case wLEVEL:
		if(new_level == WP_Init)
			return Error("cannot set level to INIT");

		if(bbs_mode) {
			if(ap->wpb == NULL)	
				return Error("must be a bbs");
			else {
				if(strcmp(ap->wpu->call, ap->wpu->home))
					return Error("must be a bbs");
				ap->wpb->level = new_level;
			}
		} else
			ap->wpu->level = new_level;
		return "OK\n";

	case wSYSOP:	new_level = WP_Sysop; break;
	case wUSER:		new_level = WP_User; break;
	case wGUESS:	new_level = WP_Guess; break;
	}

	if(*s == 0) {
		return Error("expected field to follow level");
	}

	result = parse(ap, s);
	new_level = WP_Init;
	return result;
}

char *
edit_string(struct active_processes *ap, int token, char *s)
{
	char *string, buf[256];
	int length, raise_case = FALSE;
	long *level = &(ap->wpu->level);
	int *image = &user_image;
	time_t *changed = &(ap->wpu->changed);

	switch(token) {
	case wFNAME:
		string = ap->wpu->fname;
		length = LenFNAME;
		break;
	case wHOME:
		string = ap->wpu->home;
		length = LenCALL;
		raise_case = TRUE;
		break;
	case wQTH:
		string = ap->wpu->qth;
		length = LenQTH;
		break;
	case wZIP:
		string = ap->wpu->zip;
		length = LenZIP;
		break;
	case wHLOC:
		if(*s && strcmp(ap->wpu->call, ap->wpu->home))
			return Error("must be a bbs");

		if(ap->wpb == NULL)
			return Error("no bbs entry");
		string = ap->wpb->hloc;
		length = LenHLOC;
		raise_case = TRUE;
		level = &(ap->wpb->level);
		image = &bbs_image;
		changed = NULL;
		break;
	default:
		return Error("not a string field");
	}

	if(new_level == WP_Init) {
		if(*s != 0)
			return Error("level not present could not edit field");
		sprintf(output, "%s\n", string);
		return output;
	}

		/* if the existing field is "?" than accept anything as better
		 * information. Do this by fudging the level.
		 */

	if(new_level > *level) {
		if(strcmp(string, "?"))
			return Ok("although level was too low, no change made");
		new_level = *level;
	}

	if(*s == 0) { 
		strcpy(string, "?");
	} else {

		if(token == wQTH)
			strncpy(buf, s, length);
		else
			strncpy(buf, get_string(&s), length);
		strcpy(string, buf);
		if(raise_case)
			uppercase(string);

		if(token == wHOME)
			ap->wpb = hash_get_bbs(ap->wpu->home);
		*level = new_level;
	}

	if(changed)
		*changed = Time(NULL);
	*image = DIRTY;
	return "OK\n";
}

/*ARGSUSED*/
char *
edit_number(struct active_processes *ap, int token, char *s)
{
	return "NO, not coded yet\n";
}

struct wp_user_entry *
make_user(char *call)
{
	struct wp_user_entry *wpu = malloc_struct(wp_user_entry);
	if(wpu != NULL) {
		strcpy(wpu->call, call);
		strcpy(wpu->fname, "?");
		strcpy(wpu->qth, "?");
		strcpy(wpu->zip, "?");
		strcpy(wpu->home, "?");
		wpu->firstseen = 0;
		wpu->seen = 0;
		wpu->changed = Time(NULL);
		wpu->last_update_sent = 0;
		wpu->level = WP_Init;
		user_image = DIRTY;
		hash_insert_user(call, wpu);
	}
	return wpu;
}

struct wp_bbs_entry *
make_bbs(char *call)
{
	struct wp_bbs_entry *wpb = hash_create_bbs(call);

	if(wpb != NULL) {
		strcpy(wpb->call, call);
		strcpy(wpb->hloc, "?");
		wpb->level = WP_Init;
		bbs_image = DIRTY;
	}
	return wpb;
}

char *
create_user(struct active_processes *ap, char *s)
{
	char call[80];

	strcpy(call, get_string(&s));
	uppercase(call);
	if(!iscall(call))
		return Error("invalid callsign");

	if((ap->wpu = hash_get_user(call)) == NULL)
		ap->wpu = make_user(call);
	else
		if(!bbs_mode)
			return Error("user already exists");

	if(bbs_mode) {
		strcpy(ap->wpu->home, call);
		user_image = DIRTY;

		if((ap->wpb = hash_get_bbs(call)) == NULL)
			ap->wpb = make_bbs(call);
		else
			return Error("bbs already exists");
	}

	return "OK\n";
}

char *
kill_user(char *s)
{
	char call[80];

	strcpy(call, get_string(&s));
	uppercase(call);
	if(!iscall(call))
		return Error("invalid callsign");

	if(!bbs_mode) {
		if(hash_get_user(call) == NULL)
			return Error("callsign not in database");
		if(hash_delete_user(call) == OK)
			user_image = DIRTY;
	}

	if(hash_get_bbs(call) == NULL) {
		if(bbs_mode)
			return Error("callsign not a bbs in the database");
	} else
		if(hash_delete_bbs(call) == OK)
			bbs_image = DIRTY;

	return "OK\n";
}

char *
show_user(struct active_processes *ap)
{
	char *level;

	if(bbs_mode && ap->wpb == NULL)
		return Error("no home bbs entry");
		
	switch(bbs_mode ? ap->wpb->level : ap->wpu->level) {
	case WP_Sysop:	level = "SYSOP"; break;
	case WP_User:	level = "USER"; break;
	case WP_Guess:	level = "GUESS"; break;
	case WP_Init:	level = "INIT"; break;
	default:
		level = "UNKNOWN"; break;
	}

	output[0] = 0;

	if(!bbs_mode) {
		sprintf(output, "FNAME %s\n", ap->wpu->fname);	
		sprintf(output, "%sHOME %s\n", output, ap->wpu->home);	
	}

	if(ap->wpb)
		sprintf(output, "%sHLOC %s\n", output, ap->wpb->hloc);

	if(!bbs_mode) {
		sprintf(output, "%sQTH %s\n", output, ap->wpu->qth);
		sprintf(output, "%sZIP %s\n", output, ap->wpu->zip);
	}
	sprintf(output, "%sLEVEL %s\n", output, level);
	sprintf(output, "%s.\n", output);
	return output;
}
