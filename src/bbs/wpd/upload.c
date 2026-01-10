#include <stdio.h>
#include <string.h>

#include "c_cmmn.h"
#include "tools.h"
#include "wp.h"

/*
On 910726 N6ZFJ/U @ N0ARY.#NOCAL.CA.USA.NA zip 94086 Connie Sunnyvale, CA
*/

int
different(char *s, char *p)
{
	return strcasecmp(s, p);
}

enum {
	UPD_INVALID_ON = 1,
	UPD_INVALID_DATE,
	UPD_EXPECTED_SLASH,
	UPD_INVALID_CALLSIGN,
	UPD_EXPECTED_AT,
	UPD_INVALID_HOME_CALLSIGN,
	UPD_EXPECTED_HLOC,
	UPD_EXPECTED_ZIP,
	UPD_EXPECTED_ZIP_CODE,
	UPD_EXPECTED_FIRST_NAME,
	UPD_EXPECTED_QTH,
};

static char *
desc_parse_error(int res)
{
	switch (res) {
	case UPD_INVALID_ON:
		return Ok("but invalid format [On]");
	case UPD_INVALID_DATE:
		return Ok("unparsable update date");
	case UPD_EXPECTED_SLASH:
		return Ok("expected a '/' following the call");
	case UPD_INVALID_CALLSIGN:
		return Ok("doesn't look like a valid callsign");
	case UPD_EXPECTED_AT:
		return Ok("expected '@' before home bbs");
	case UPD_INVALID_HOME_CALLSIGN:
		return Ok("homebbs doesn't look like a valid callsign");
	case UPD_EXPECTED_HLOC:
		return Ok("unparseable HLOC in home BBS");
	case UPD_EXPECTED_ZIP:
		return Ok("expected the word zip, not found");
	case UPD_EXPECTED_ZIP_CODE:
		return Ok("expected zip code, not found");
	case UPD_EXPECTED_FIRST_NAME:
		return Ok("expected first name, not found");
	case UPD_EXPECTED_QTH:
		return Ok("expected qth, not found");
	default:
		return Ok("unknown parse failure!");
	}

	return NULL;
}

static int
parse_upload_line(char *s, struct wp_user_upload_entry *wp, int *have_hloc)
{
	char *p, *call;

	/* On 910726 N6ZFJ/U @ N0ARY.#NOCAL.CA.USA.NA zip ... */
	if(strncmp(s, "On ", 3))
		return UPD_INVALID_ON;

	if((p = (char *)index(s, '\n')) != NULL)
		*p = 0;

	s += 3;
	/* 910726 N6ZFJ/U @ N0ARY.#NOCAL.CA.USA.NA zip 94086 ... */
	if (safe_udate2time(&wp->changed, &s) != 0)
		return UPD_INVALID_DATE;

	/* N6ZFJ/U @ N0ARY.#NOCAL.CA.USA.NA zip 94086 Connie ... */
	call = s;
	if((p = (char*)index(s, '/')) == NULL)
		return UPD_EXPECTED_SLASH;

	*p++ = 0;

	if (safe_get_call(wp->call, &call, sizeof(wp->call)) != 0)
		return UPD_INVALID_CALLSIGN;

	/* U @ N0ARY.#NOCAL.CA.USA.NA zip 94086 Connie Sunnyvale, CA */
	switch(*p++) {
	case 'U':
		wp->level = WP_User; break;
	default:
		wp->level = WP_Guess; break;
	}
	
	NextChar(p);

	/* @ N0ARY.#NOCAL.CA.USA.NA zip 94086 Connie Sunnyvale, CA */
	if(*p != '@')
		return UPD_EXPECTED_AT;

	p++;
	NextChar(p);

	/* N0ARY.#NOCAL.CA.USA.NA zip 94086 Connie Sunnyvale, CA */
	if (safe_get_call(wp->home, &p, sizeof(wp->home)) != 0)
		return UPD_INVALID_HOME_CALLSIGN;

	/* .#NOCAL.CA.USA.NA zip 94086 Connie Sunnyvale, CA */
	if(*p == '.') {
		p++;

		/* #NOCAL.CA.USA.NA zip 94086 Connie Sunnyvale, CA */
		if (safe_get_string(wp->hloc, &p, sizeof(wp->hloc)) != 0)
			return UPD_EXPECTED_HLOC;

		uppercase(wp->hloc);
		*have_hloc = 1;
	}

	/* skip forward to "zip" */
	if((p = (char*)strstr(p, "zip")) == NULL)
		return UPD_EXPECTED_ZIP;

	/* zip 94086 Connie Sunnyvale, CA */
	get_string(&p);

	/* 94086 Connie Sunnyvale, CA */
	if (safe_get_string(wp->zip, &p, sizeof(wp->zip)) != 0)
		return UPD_EXPECTED_ZIP_CODE;

	/* Connie Sunnyvale, CA */
	if (safe_get_string(wp->fname, &p, sizeof(wp->fname)) != 0)
		return UPD_EXPECTED_FIRST_NAME;

	/* Sunnyvale, CA */
	if(*p == 0)
		return UPD_EXPECTED_QTH;

	strlcpy(wp->qth, p, sizeof(wp->qth));

	/* trim any spaces off the end of the qth */
	size_t len = strlen(wp->qth);
	char *q = &wp->qth[len-1];
	while(len--) {
		if(!isspace(*q))
			break;
		*q-- = 0;
	}

	/* All ok! */
	uppercase(wp->call);
	uppercase(wp->home);

	return 0;
}

char *
upload(char *s)
{
	struct wp_user_entry ue, *exist;
	struct wp_user_upload_entry up;
	struct wp_bbs_entry *wpb;
	int res, level, got_hloc;

	res = parse_upload_line(s, &up, &got_hloc);
	if (res != 0)
		return desc_parse_error(res);

	if (got_hloc) {
		char *home = up.home;
		if((wpb = hash_get_bbs(home)) == NULL && iscall(home)) {
			/* homebbs is a callsign, but not known to us, lets
			 * create it at the guess level.
			 */
			struct wp_user_entry *wpu = hash_get_user(home);
			if(wpu == NULL) {
				if((wpu = make_user(home)) != NULL) {
					strcpy(wpu->home, home);
					wpu->changed = up.changed;
				}
			}

			if(wpu != NULL)
				if((wpb = make_bbs(home)) != NULL) {
					strlcpy(wpb->hloc, up.hloc, LenHLOC);
				}
		}
	}

	int update = FALSE;

	if ((exist = hash_get_user(up.call)) != NULL) {
		if(up.home[0] == '?')
			strlcpy(up.home, exist->home, sizeof(up.home));
		if(up.zip[0] == '?')
			strlcpy(up.zip, exist->zip, sizeof(up.zip));
		if(up.fname[0] == '?')
			strlcpy(up.fname, exist->fname, sizeof(up.fname));
		if(up.qth[0] == '?')
			strlcpy(up.qth, exist->qth, sizeof(up.qth));

		int more_recent = up.changed > exist->changed;
		int more_trustworthy = up.level < exist->level;
		int as_trustworthy = up.level == exist->level;

		if((as_trustworthy && more_recent) || more_trustworthy) {
			if(different(exist->home, up.home))
				update = TRUE;
			if(strcmp(exist->zip, up.zip))
				update = TRUE;
			if(strcmp(exist->fname, up.fname))
				update = TRUE;
			if(strcmp(exist->qth, up.qth))
				update = TRUE;
		}

		if (update) {
			ue.firstseen = exist->firstseen;
			ue.seen = exist->seen;
			ue.last_update_sent = exist->last_update_sent;
		}
	} else {
		ue.firstseen = up.changed;
		ue.seen = up.changed;
		ue.last_update_sent = 0;
		update = TRUE;
	}

	if(update == TRUE) {
		strlcpy(ue.call, up.call, sizeof(ue.call));
		strlcpy(ue.home, up.home, sizeof(ue.home));
		strlcpy(ue.zip, up.zip, sizeof(ue.zip));
		strlcpy(ue.fname, up.fname, sizeof(ue.fname));
		strlcpy(ue.qth, up.qth, sizeof(ue.qth));
		ue.level = up.level;
		ue.changed = up.changed;
		hash_insert_or_update_user(&ue);
		user_image = DIRTY;
	}

	return "OK\n";
}
