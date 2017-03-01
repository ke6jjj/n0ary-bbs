#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "gate.h"

static char *age_msg[] = {
  "\n",
  "This is an automatically generated message from the packet<>internet\n",
  "gateway at the bbs. You have the following addresses registered for\n",
  "mail going from internet to packet. If an address hasn't been seen\n",
  "on an incoming message in a significant amount of time. If the address\n",
  "unused it will be purged from the gateway. Your active addresses will\n",
  "remain in intact.\n",
  "\n",
  NULL };

void
age(struct active_processes *ap)
{
	time_t t = Time(NULL);
	struct gate_entry *g = GateList;
	char buf[256];

	time_t warn = t - Gated_Age_Warn;
	time_t kill = t - (Gated_Age_Kill - Gated_Age_Warn);

		/* first purge all old accounts that have already
		 * received their warning...
		 */

	while(g) {
		struct gate_entry *e = g;
		int restart = FALSE;
		while(e) {
			if(e->warn_sent && (e->warn_sent < kill)) {
				if(ap) {
					sprintf(buf, "KILL %s ", time2date(e->seen));
					sprintf(buf, "%s%s %s\t%s\n", buf,
						time2date(e->warn_sent), e->call, e->addr);
					write(ap->fd, buf, strlen(buf));
				}
				if(delete_entry(e->call, e->addr) == OK) {
					restart = TRUE;
					break;
				} else
					if(ap) {
						sprintf(buf, "Problem deleting %s %s\n",
							e->call, e->addr);
						write(ap->fd, buf, strlen(buf));
					}
			}
			e = e->chain;
		}

		if(restart)
			g = GateList;
		else
			g = g->next;
	}

		/* the first pass should have purged all the really old
		 * accounts. Now we want to find accounts that have just
		 * crossed the warning threshold.
		 */

	g = GateList;
	while(g) {
		struct gate_entry *e = g;
		int cnt = 0;

			/* let's see if the user has any old addresses */
		while(e) {
			if(e->warn_sent == 0)
				if(e->seen < warn) {
					cnt++;
					e->warn_sent = t;
				}
			e = e->chain;
		}

		e = g;
		if(cnt) {
			int i;
			for(i=0; age_msg[i]; i++) {
				if(ap)
					write(ap->fd, age_msg[i], strlen(age_msg[i]));
			}
				
			while(e) {
				sprintf(buf, "%s %s %s\t%s\n", e->warn_sent ? "=>":"  ",
					time2date(e->seen), e->call, e->addr);
				if(ap)
					write(ap->fd, buf, strlen(buf));
				e = e->chain;
			}	
		}
		g = g->next;
	}
}
