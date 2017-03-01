#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "rfc822.h"
#include "msgd.h"

void
SetMsgActive(struct msg_dir_entry *m)
{
	m->flags &= ~MsgStatusMask;
	m->flags |= MsgActive;
	m->odate = m->kdate = 0;
	rfc822_append(m->number, rCREATE, rfc822_get_field(m->number, rCREATE));
}

void
SetMsgOld(struct msg_dir_entry *m)
{
	m->flags &= ~MsgStatusMask;
	m->flags |= (MsgOld | MsgActive);
}

void
SetMsgHeld(struct msg_dir_entry *m)
{
		/* caller should handle rfc822 generation */
	m->flags &= ~MsgStatusMask;
	m->flags |= (MsgHeld | MsgActive);
}

void
SetMsgKilled(struct msg_dir_entry *m)
{
	char buf[80];

	m->flags &= ~(MsgStatusMask|MsgPending);
	m->flags |= MsgKilled;
	m->fwd_cnt = 0;
	m->kdate = Time(NULL);
	fwddir_kill(m->number, NULL);

	sprintf(buf, "%"PRTMd, m->kdate);
	rfc822_append(m->number, rKILL, buf);
}

void
SetMsgImmune(struct msg_dir_entry *m)
{
	m->flags &= ~MsgStatusMask;
	m->flags |= (MsgActive | MsgImmune);
	rfc822_append(m->number, rIMMUNE, "ON");
}

void
SetMsgRefresh(struct msg_dir_entry *m)
{
	char buf[80];

	m->flags &= ~MsgStatusMask;
	m->flags |= MsgActive;
	m->cdate = Time(NULL);
	m->odate = 0;
	m->kdate = 0;

	sprintf(buf, "%"PRTMd, m->cdate);
	rfc822_append(m->number, rCREATE, buf);
}

void
SetMsgPersonal(struct msg_dir_entry *m)
{
	m->flags &= ~MsgTypeMask;
	m->flags |= MsgPersonal;
	rfc822_append(m->number, rTYPE, "P");
}

void
SetMsgBulletin(struct msg_dir_entry *m)
{
	m->flags &= ~MsgTypeMask;
	m->flags |= MsgBulletin;
	rfc822_append(m->number, rTYPE, "B");
}

void
SetMsgNTS(struct msg_dir_entry *m)
{
	m->flags &= ~MsgTypeMask;
	m->flags |= MsgNTS;
	rfc822_append(m->number, rTYPE, "T");
}

void
SetMsgSecure(struct msg_dir_entry *m)
{
	m->flags &= ~MsgTypeMask;
	m->flags |= MsgSecure;
	m->flags |= MsgPersonal;
	rfc822_append(m->number, rTYPE, "S");
}

void
SetMsgPassword(struct msg_dir_entry *m, char *s)
{
	SetMsgSecure(m);
	m->flags |= MsgPassword;
	strcpy(m->passwd, s);
	rfc822_append(m->number, rPASSWORD, s);
}

void
SetMsgPending(struct msg_dir_entry *m)
{
	m->flags |= MsgPending;
}

void
ClrMsgPending(struct msg_dir_entry *m)
{
	m->flags &= ~MsgPending;
}

void
SetMsgNoForward(struct msg_dir_entry *m)
{
	m->flags &= ~MsgPending;
}

void
SetMsgNoFwd(struct msg_dir_entry *m)
{
	m->flags &= ~MsgPending;
	m->flags |= MsgNoFwd;
}

void
SetMsgLocal(struct msg_dir_entry *m)
{
	m->flags &= ~MsgKindMask;
	m->flags |= MsgLocal;
}


void
SetMsgCall(struct msg_dir_entry *m)
{
	m->flags &= ~MsgKindMask;
	m->flags |= MsgCall;
}

void
SetMsgCategory(struct msg_dir_entry *m)
{
	m->flags &= ~MsgKindMask;
	m->flags |= MsgCategory;
}

void
SetMsgRead(struct msg_dir_entry *m, char *call)
{
	struct text_line *tl = m->read_by;
	while(tl) {
		if(!strcmp(tl->s, call))
			return;
		NEXT(tl);
	}

	if(!strcmp(m->to.name.str, call))
		m->flags |= MsgRead;

	m->read_cnt++;
	textline_append(&(m->read_by), call);
	rfc822_append(m->number, rREADBY, call);
	build_list_text(m);
}

char *
edit_message(struct active_processes *ap, struct msg_dir_entry *msg, char *s)
{
	int was_bulletin = IsMsgBulletin(msg);
	int field;
	char bid[80], buf[80];

			/* not all fields should be editable. Such as KILL which is
			 * a command itself.
			 */
	strcpy(bid, msg->bid);
	field = rfc822_parse(msg, s);

	switch(field) {
	case rTYPE:
	case rTO:
	case rFROM:
	case rSUBJECT:
	case rPASSWORD:
	case rLIVE:
		break;
	case rBID:
		if(!strcmp(msg->bid, "$")) {
			sprintf(msg->bid, "%ld_%s", msg->number, Bbs_Call);
			rfc822_gen(rBID, msg, buf, 80);
			s = buf;
		}
		if(bid_chk(msg->bid)) {
			strcpy(msg->bid, bid);
			return Error("Duplicate BID");
		}
		bid_add(msg->bid);
		break;
	case ERROR:
		return Error("Could not parse RFC822 field");
	default:
		return Error("Not an editable field");
	}

	rfc822_append_complete(msg->number, s);

			/* at this point the new rfc822 field has been determined and
			 * written into the file. We should now see if the field that
			 * changed alters things like forwarding, etc.
			 */

	switch(field) {
	case rTYPE:
		if(was_bulletin)
			remove_from_groups(msg);
		if(msg->flags & MsgSecure) {
			fwddir_kill(msg->number, NULL);
			break;
		}
	case rTO:
		if(IsMsgBulletin(msg)) {
			remove_from_groups(msg);
			set_groups(msg);
		}
		fwddir_kill(msg->number, NULL);
		set_forwarding(ap, msg, FALSE);
		break;
	}

			/* now we have to notify the users if the changes affect
			 * their existing lists.
			 */

	switch(field) {
	case rBID:
	case rFROM:
	case rSUBJECT:
	case rTO:
	case rTYPE:
	case rPASSWORD:
		build_list_text(msg);
		break;
	}
	return "OK\n";
}
