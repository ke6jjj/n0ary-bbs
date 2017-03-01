/* see comment at the head of ax_mbx.c - Bdale */
/* Defines for the ax.25 mailbox facility */

/*defines that control forwarder states */


struct mboxsess{			/*mailbox session structure*/
	int pid;				/*id of bbs process*/
	int spawned;			/*did we create the process?*/

	int fd;
	int socket;
	int port;

	int bytes;				/*number of bytes for bbs to send at one time*/
	int byte_cnt;
	struct ax25_cb *axbbscb; /*axp associated with this structure*/
	struct ax25_addr *orig;
	char *p, buf[1024];
	char call[7];
	char command[10][80];
	char cmd_cnt;
	char cmd_state;
	struct mboxsess *next;	/*pointer to next session*/
};

#define NULLMBS  (struct mboxsess *)0
#define NULLFWD  (struct ax25_cb *)0

extern struct mboxsess
	*base;

extern int
	init_ax_control(char *c_bindaddr, int c_port, char *m_bindaddr,
		int m_port);

extern void
	ax_control(void),
	axchk(void),
	mbx_state(struct ax25_cb *axp, int old, int new),
	mbx_incom(struct ax25_cb *axp, int cnt);
