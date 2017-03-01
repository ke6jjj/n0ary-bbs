#include <stdio.h>
#include <sys/wait.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "smtp.h"
#include "gateway.h"

static int
termination(char *s)
{
	if(!strncmp(s, "/EX", 3))
		return TRUE;
	if(!strncmp(s, "/ex", 3))
		return TRUE;
	return FALSE;
}

static int
wait_for(int fd, char *term)
{
	char buf[4096];
	char *s;

	if((s = (char*)re_comp(term)) != NULL)
		exit(1);

	do {
		s = buf;
		while(TRUE) {
			if(read(fd, s, 1) <= 0)
				exit(0);
			if(*s == '\n') {
				*s = 0;
				break;
			}
			s++;
		}
	} while(re_exec(buf) == FALSE);
	return OK;
}

int
import_generate(FILE *fp)
{
	int to_bbs[2], to_gate[2];
	char buf[1024];
	int done = FALSE;
	char cmd[256];

	fgets(buf, 1024, fp);
	buf[strlen(buf)-1] = 0;

		/* if we send a message out and it comes bouncing back it will
		 * look as though it is coming from IPGATE. Let's skip it here
		 */

	if(!strcmp(buf, "IPGATE"))
		return TRUE;

	if(pipe(to_bbs) < 0)
		return ERROR;

	if(pipe(to_gate) < 0) {
		close(to_bbs[0]);
		close(to_bbs[1]);
		return ERROR;
	}

	if(fork() == 0) {
		close(to_bbs[1]);
		close(0);
		dup(to_bbs[0]);
		close(to_bbs[0]);

		close(to_gate[0]);
		close(1);
		dup(to_gate[1]);
		close(to_gate[1]);

				/* should I be logging in as the bbs in question? A
				 * problem will arise if they log in as a regular user
				 * some time. Also ECHO must be off on the user account.
				 */

		sprintf(cmd, "%s/b_bbs", Bin_Dir);
		execl(cmd, "b_bbs", "-v", "SMTP", "-t", "2", buf, 0);
		exit(1);
	}

	close(to_bbs[0]);
	close(to_gate[1]);

	wait_for(to_gate[0], ">$");

		/* become a bbs */
	sprintf(cmd, "%s\n", build_sid());
	write(to_bbs[1], cmd, strlen(cmd));
	wait_for(to_gate[0], ">$");

	while(!done) {
		char buf[SMTP_BUF_SIZE];
		char resp[1024];

		if(fgets(buf, SMTP_BUF_SIZE, fp) == 0)
			break;

		if(buf[0] == '\n')
			continue;

		if(buf[0] == '.') {
			done = TRUE;
			break;
		}

		if(buf[0] != 'S' && buf[0] != 's') {
								/* W2XO has a problem with tacking
								 * non-messages onto the end of valid
								 * messages. This is identified by a
								 * /EX not being followed by a send
								 * command. Let's just accept.
								 */
#if 1
			done = TRUE;
#else
			done = ERROR;
#endif
			break;
		}

			/* at this time we should be at a valid bbs send command */

		if(write(to_bbs[1], buf, strlen(buf)) < 0) {
			done = ERROR;
			break;
		}

		read_line(to_gate[0], resp, 1024);
		if(resp[0] == 'O') {
			while(!done) {
				if(fgets(buf, SMTP_BUF_SIZE, fp) == 0) {
					done = ERROR;
					break;
				}

				if(buf[0] == '.' && buf[1] == 0) {
					write(to_bbs[1], "/EX\n", 4);
					done = TRUE;
					break;
				}

				if(write(to_bbs[1], buf, strlen(buf)) < 0) {
					done = ERROR;
					break;
				}

				if(termination(buf))
					break;

			}

		} else {
			while(!done) {
				if(fgets(buf, SMTP_BUF_SIZE, fp) == 0) {
					done = ERROR;
					break;
				}
				if(buf[0] == '.' && buf[1] == 0) {
					done = TRUE;
					break;
				}

				if(termination(buf))
					break;
			}
		}

		if(done == ERROR)
			break;

		wait_for(to_gate[0], ">$");
	}

	if(done != ERROR)
		write(to_bbs[1], "EXIT\n", 5);
	sleep(2);
	close(to_gate[0]);
	close(to_bbs[1]);
	wait3(NULL, WNOHANG, NULL);
	if(done == ERROR)
		return ERROR;
	return OK;
}

