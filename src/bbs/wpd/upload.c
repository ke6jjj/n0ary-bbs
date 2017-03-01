#include <stdio.h>
#include <string.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "wp.h"

/*
On 910726 N6ZFJ/U @ N0ARY.#NOCAL.CA.USA.NA zip 94086 Connie Sunnyvale, CA
*/

int
different(char *s, char *p)
{
	char s0[80], s1[80];
	strcpy(s0, s);
	strcpy(s1, p);
	uppercase(s0);
	uppercase(s1);

	return strcmp(s0, s1);
}

char *
upload(char *s)
{
	struct wp_user_entry *wp;
	struct wp_bbs_entry *wpb;

	char *p, *call;
	char home[80], hloc[80], zip[80], fname[80], qth[80];
	int level;
	int changed = FALSE;
	time_t t;

	strcpy(home, "?");
	strcpy(hloc, "?");
	strcpy(zip, "?");
	strcpy(fname, "?");
	strcpy(qth, "?");

/* On 910726 N6ZFJ/U @ N0ARY.#NOCAL.CA.USA.NA zip 94086 Connie Sunnyvale, CA */
	if(strncmp(s, "On ", 3))
		return Ok("but invalid format [On]");

	if((p = (char *)index(s, '\n')) != NULL)
		*p = 0;

	s += 3;
/* 910726 N6ZFJ/U @ N0ARY.#NOCAL.CA.USA.NA zip 94086 Connie Sunnyvale, CA */
	if(*s != '9')
		return Ok("expected this decade [On 9]");
	t = udate2time(get_string(&s));

/* N6ZFJ/U @ N0ARY.#NOCAL.CA.USA.NA zip 94086 Connie Sunnyvale, CA */
	call = s;
	if((p = (char*)index(s, '/')) == NULL)
		return Ok("expected a '/' following the call");

	*p++ = 0;
	if(!iscall(call))
		return Ok("doesn't look like a valid callsign");

	wp = hash_get_user(call);
	if(wp == NULL) {
		wp = make_user(call);
		wp->changed = t;
	}

/* U @ N0ARY.#NOCAL.CA.USA.NA zip 94086 Connie Sunnyvale, CA */
	switch(*p++) {
	case 'U':
		level = WP_User; break;
	default:
		level = WP_Guess; break;
	}
	
	NextChar(p);

/* @ N0ARY.#NOCAL.CA.USA.NA zip 94086 Connie Sunnyvale, CA */
	if(*p != '@')
		return Ok("expected '@' before home bbs");

	p++;
	NextChar(p);

/* N0ARY.#NOCAL.CA.USA.NA zip 94086 Connie Sunnyvale, CA */
	strcpy(home, get_call(&p));
	home[LenCALL-1] = 0;
	if(!iscall(home))
		return Ok("homebbs doesn't look like a valid callsign");

/* .#NOCAL.CA.USA.NA zip 94086 Connie Sunnyvale, CA */
	if(*p == '.') {
		p++;

/* #NOCAL.CA.USA.NA zip 94086 Connie Sunnyvale, CA */
		if((wpb = hash_get_bbs(home)) == NULL)
				/* homebbs is not known to us, lets create it
				 * but only at the guess level and only if it is a
				 * callsign.
				 */

			if(iscall(home)) {
				struct wp_user_entry *wpu = hash_get_user(home);
				if(wpu == NULL) {
					if((wpu = make_user(home)) != NULL) {
						strcpy(wpu->home, home);
						wpu->changed = t;
					}
				}

				if(wpu != NULL)
					if((wpb = make_bbs(home)) != NULL) {
						strncpy(wpb->hloc, get_string(&p), LenHLOC);
						wpb->hloc[LenHLOC-1] = 0;
					}
			}
	}

		/* skip forward to "zip" */
	if((p = (char*)strstr(p, "zip")) == NULL)
		return Ok("expected the word zip, not found");

/* zip 94086 Connie Sunnyvale, CA */
	get_string(&p);

/* 94086 Connie Sunnyvale, CA */
	if(*p == 0)
		return Ok("expected zip code, not found");

	strcpy(zip, get_string(&p));
	zip[LenZIP-1] = 0;

/* Connie Sunnyvale, CA */
	if(*p == 0)
		return Ok("expected first name, not found");
	strcpy(fname, get_string(&p));
	fname[LenFNAME-1] = 0;

/* Sunnyvale, CA */
	if(*p == 0)
		return Ok("expected qth, not found");

	strcpy(qth, p);
	qth[LenQTH-1] = 0;

	{
			/* trim any spaces off the end of the qth */
		int len = strlen(qth);
		char *q = &qth[len-1];
		while(len--) {
			if(!isspace(*q))
				break;
			*q-- = 0;
		}
	}

		/* first, if we have any question marks in our records use
		 * whatever is presented here.
		 */

	if(wp->home[0] == '?' && home[0] != '?') {
		strcpy(wp->home, home);
		changed = TRUE;
	}

	if(wp->zip[0] == '?' && zip[0] != '?') {
		strcpy(wp->zip, zip);
		changed = TRUE;
	}

	if(wp->fname[0] == '?' && fname[0] != '?') {
		strcpy(wp->fname, fname);
		changed = TRUE;
	}

	if(wp->qth[0] == '?' && qth[0] != '?') {
		strcpy(wp->qth, qth);
		changed = TRUE;
	}

	if((level == wp->level && t > wp->changed) || level < wp->level) {
		if(different(wp->home, home)) {
			strcpy(wp->home, home);
			changed = TRUE;
		}
		if(zip[0] != '?' && different(wp->zip, zip)) {
			strcpy(wp->zip, zip);
			changed = TRUE;
		}
		if(fname[0] != '?' && different(wp->fname, fname)) {
			strcpy(wp->fname, fname);
			changed = TRUE;
		}
		if(qth[0] != '?' && different(wp->qth, qth)) {
			strcpy(wp->qth, qth);
			changed = TRUE;
		}

		if(changed)
			wp->level = level;
	}

	if(changed == TRUE) {
		user_image = DIRTY;
		wp->changed = t;
	}
	return "OK\n";
}
