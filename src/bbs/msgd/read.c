#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "msgd.h"

static int
compar(const void *x, const void *y)
{
	const int *i = x, *j = y;

	return (*i - *j);
}

int
build_msgdir(void)
{
	struct dirent *dp;
	DIR *dirp = opendir(Msgd_Body_Path);
	int i, cnt = 0;
	int *fn, *p;
	long now = Time(NULL);

	if (dirp == NULL)
		return ERROR;

	for(dp=readdir(dirp); dp!=NULL; dp=readdir(dirp))
		if(isdigit(dp->d_name[0]))
			cnt++;

	rewinddir(dirp);

	p = fn = (int *)calloc(sizeof(int), cnt);

	for(dp=readdir(dirp); dp!=NULL; dp=readdir(dirp))
		if(isdigit(dp->d_name[0]))
			*p++ = atoi(dp->d_name);		
	closedir(dirp);

	qsort(fn, cnt, sizeof(int), compar);

	fwddir_open();

	for(i=0; i<cnt; i++) {
		struct msg_dir_entry *msg = append_msg_list();

		if(dbug_level & dbgVERBOSE)
			if(i%200 == 0) {
				printf("%d    \r", cnt - i); fflush(stdout);
			}

		msg->number = fn[i];
		if(rfc822_decode_fields(msg) == OK) {
			if(IsMsgBulletin(msg))
				set_groups(msg);
			msg->fwd_cnt = fwddir_check(msg->number);
			msg->flags &= ~MsgPending;
			if(msg->fwd_cnt)
				msg->flags |= MsgPending;
		}
		build_list_text(msg);
	}

	return OK;
}

void
read_messages(int key, struct active_processes *ap, struct msg_dir_entry *msg,
	char *by)
{
	FILE *fp = open_message(msg->number);
	int in_header = TRUE;
	char buf[128];

	while(fgets(buf, 127, fp)) {
		if(!strcmp(buf, "/EX\n"))
			break;

		buf[127] = 0;
		if(in_header) {
			if(buf[0] != 'R' || buf[1] != ':') {
				in_header = FALSE;
				if(buf[0] == '\n' && key == mREAD)
					continue;
			}
		}

		if(!in_header || key == mREADH) {
			if(buf[0] == '.')
				socket_raw_write(ap->fd, ">");
			socket_raw_write(ap->fd, buf);
		}
	}
	close_message(fp);
	if(by) {
		uppercase(by);
		SetMsgRead(msg, by);
	} else
		SetMsgRead(msg, ap->call);
	build_list_text(msg);
}

void
read_messages_rfc(struct active_processes *ap, struct msg_dir_entry *msg)
{
	FILE *fp = open_message(msg->number);
	char buf[128];

	while(fgets(buf, 127, fp))
		if(!strcmp(buf, "/EX\n"))
			break;

	while(fgets(buf, 127, fp)) {
		buf[127] = 0;
		socket_raw_write(ap->fd, buf);
	}
	close_message(fp);
}

void
msg_body_kill(int num)
{
	char fn[80];
	sprintf(fn, "%s/%05d", Msgd_Body_Path, num);
	unlink(fn);
}

int
msg_body_rename(int orig, int new)
{
	char ofn[80], nfn[80];
	sprintf(ofn, "%s/%05d", Msgd_Body_Path, orig);
	sprintf(nfn, "%s/%05d", Msgd_Body_Path, new);

	return rename(ofn, nfn);
}

