
#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "wp.h"

char *
report_entry(char *call)
{
	struct wp_user_entry *wpu;
	struct wp_bbs_entry *wpb;
	static char output[1024];
	char home[80];

	output[0] = 0;
	if((wpu = hash_get_user(call)) == NULL)
		sprintf(output, "NO User %s in local white pages database\n", call);

	else {
		if((wpb = hash_get_bbs(wpu->home)) == NULL)
			sprintf(home, "%s[don't know where this bbs is]", wpu->home);
		else
			sprintf(home, "%s.%s", wpb->call, wpb->hloc);
	
		sprintf(output, "%s @ %s %s %s %s\n", wpu->call, home, wpu->fname,
			wpu->zip, wpu->qth);
	}

	return output;
}

char *
report_update(struct active_processes *ap)
{
	if(ap->wpb == NULL)
		return Error("invalid homebbs");

	sprintf(output, "%d %d %d On %s %s/%c @ %s.%s zip %s %s %s\n",
		ap->wpu->changed, ap->wpu->seen, ap->wpu->last_update_sent,
		time2udate((ap->wpu->seen > ap->wpu->changed)?ap->wpu->seen:ap->wpu->changed),
		ap->wpu->call,
		(ap->wpu->level == WP_Guess) ? 'G' : 'U',
		ap->wpb->call, ap->wpb->hloc,
		ap->wpu->zip, ap->wpu->fname, ap->wpu->qth);

	return output;
}
