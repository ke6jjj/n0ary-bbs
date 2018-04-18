#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

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
read_message(int key, struct active_processes *ap, struct msg_dir_entry *msg,
	char *by)
{
	FILE *fp = open_message(msg->number);
	int in_header = TRUE;
	int nl, next_nl;
	char buf[128];
	size_t len;

	for (nl = TRUE,next_nl=0; fgets(buf, sizeof(buf), fp); nl = next_nl) {
		len = strlen(buf);
		/* Remember if this line overflowed. */
		next_nl = buf[len-1] == '\n';

		if(nl && !strcmp(buf, "/EX\n"))
			break;

		if(in_header) {
			if(nl && (buf[0] != 'R' || buf[1] != ':')) {
				in_header = FALSE;
				if(buf[0] == '\n' && key == mREAD)
					continue;
			}
		}

		if(!in_header || key == mREADH) {
			if(nl && buf[0] == '.')
				socket_raw_write(ap->fd, ".");
			socket_raw_write(ap->fd, buf);
		}
	}

	if (!next_nl) {
		/* File ended without a newline! */
		socket_raw_write(ap->fd, "\n");
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
read_message_rfc(struct active_processes *ap, struct msg_dir_entry *msg)
{
	FILE *fp = open_message(msg->number);
	char buf[128];
	int had_nl;
	size_t len;

	rfc822_skip_to(fp);

	for (had_nl = 0; fgets(buf, sizeof(buf), fp);) {
		socket_raw_write(ap->fd, buf);
		len = strlen(buf);
		had_nl = buf[len-1] == '\n';
	}

	if (!had_nl) {
		/* File ended without a newline! */
		socket_raw_write(ap->fd, "\n");
	}

	close_message(fp);
}

void
msg_body_kill(int num)
{
	char fn[PATH_MAX], nfn[PATH_MAX];
	snprintf(fn, sizeof(fn), "%s/%05d", Msgd_Body_Path, num);
	if (Msgd_Archive_Path != NULL) {
		snprintf(nfn, sizeof(nfn), "%s/%05d.%"PRTMd, Msgd_Archive_Path,
			num, time(NULL));
		rename(fn, nfn);
	} else {
		unlink(fn);
	}
}

int
msg_body_rename(int orig, int new)
{
	char ofn[PATH_MAX], nfn[PATH_MAX];
	snprintf(ofn, sizeof(ofn), "%s/%05d", Msgd_Body_Path, orig);
	snprintf(nfn, sizeof(nfn), "%s/%05d", Msgd_Body_Path, new);

	return rename(ofn, nfn);
}

