#include <stdio.h>
#include <dirent.h>
#include <time.h>

#include "config.h"
#include "user.h"

#define UserOnTnc144        0x00100000
#define UserOnTnc220        0x00200000
#define UserOnTnc440        0x00400000
#define UserOnPhoneSLOW     0x01000000
#define UserOnPhoneFAST     0x02000000
#define UserOnConsole       0x08000000


#define NEWUSRPATH "/u2/bbsdir/USER"
struct old_user {
	long	flags;
	long	rt_flags;
	long	allowed;
	long	number;
	long	time_stamp;
	long	connect_via;
	long	fwd_mask;
	long	last_msg;
	char	spare2[20];
				/* White Pages Info */
	struct call_and_checksum call;
	char	fname[LenFNAME];
	char	qth[LenQTH];
	char	zip[LenZIP];
	char	homebbs[LenHOME];
				/* UUCP forwarding address */
	char	uucp[LenEMAIL];
	char	freq[LenFREQ];
				/* Local Database Info */	
	char	lname[LenLNAME];
	char	phone[LenPHONE];
	char	tnc[LenEQUIP];
	char	computer[LenEQUIP];
	char	rig[LenEQUIP];
	char	software[LenEQUIP];
	char	spare3[20];
				/* List Categories Info */
	struct call_and_checksum
		include[20], exclude[20];	
				/* Terminal parameters */
	long	term;
	long	lines;
				/* User Parameters */
	char	macro[10][LenMACRO];
	char	password[LenPASSWD];
				/* History */
	char	padding[2];
	struct {
		long	firstseen;
		long	lastseen;
		long	count;
	} connect[6];

	long	counts[10];
				/* Runtime variables */
	time_t	last_activity;

	char	spare4[252];
};

#define	SizeofOldUser		sizeof(struct old_user)

#define	UserSysop			0x0000010
#define	UserApproved		0x0000020
#define	UserSuspect			0x0000040
#define	UserNonHam			0x0000080
#define UserRestricted		0x0100000
#define UserTypeMask		0x01000F0

#define	UserHelp0			0x0000001
#define	UserHelp1			0x0000002
#define	UserHelp2			0x0000004
#define	UserHelp3			0x0000008
#define UserHelpMask		0x000000F

#define	UserMsgAscending	0x0001000
#define	UserNewlineReqd		0x0002000
#define	UserLogging			0x0004000
#define	UserImmune			0x0008000

#define UserHalfDuplex		0x0010000
#define UserAutoSignature	0x0020000
#define UserUUCPForward		0x0040000
#define UserVacation		0x0080000

#define UserRegExp			0x0100000

bad_char(char *s)
{
	while(*s) {
		if(!isalnum(*s))
			return ERROR;

		if(isalpha(*s))
			if(islower(*s))
				return ERROR;
		s++;
	}
	return OK;
}

int
main()
{
	struct dirent *dp;
	struct old_user ou;
	struct UserInformation nu;
	char fn[80];
	DIR *dirp;
	FILE *fp;
	int i, j;

	dirp = opendir(USRPATH);
	for(dp=readdir(dirp); dp!=NULL; dp=readdir(dirp))
		if(isalnum(dp->d_name[0])) {
			struct UserInformation info;
			struct UserDirectory *dir;

			if(bad_char(dp->d_name))
				continue;

			sprintf(fn, "%s/%s", USRPATH, dp->d_name);
			if((fp = fopen(fn, "r")) == NULL) {
				printf("problem opening %s\n", dp->d_name);
				continue;
			}

			if(fread(&ou, SizeofOldUser, 1, fp) == 0) {
				printf("kill %s, incomplete file or couldn't read\n", dp->d_name);
				fclose(fp);
				continue;
			}

			fclose(fp);

			bzero(&nu, SizeOfUserInfo);

			if(ou.flags & UserSysop) nu.sysop = TRUE;
			if(ou.flags & UserApproved) nu.approved = TRUE;
			if(ou.flags & UserSuspect) nu.approved = FALSE;
			if(ou.flags & UserNonHam) nu.nonham = TRUE;
			if(ou.flags & UserRestricted) nu.approved = FALSE;
			if(ou.flags & UserHelp0) nu.help = 0;
			if(ou.flags & UserHelp1) nu.help = 1;
			if(ou.flags & UserHelp2) nu.help = 2;
			if(ou.flags & UserHelp3) nu.help = 3;
			if(ou.flags & UserMsgAscending) nu.ascending = TRUE;
			if(ou.flags & UserNewlineReqd) nu.newline = TRUE;
			if(ou.flags & UserLogging) nu.logging = TRUE;
			if(ou.flags & UserImmune) nu.immune = TRUE;
			if(ou.flags & UserHalfDuplex) nu.halfduplex = TRUE;
			if(ou.flags & UserAutoSignature) nu.signature = TRUE;
			if(ou.flags & UserUUCPForward) nu.email = TRUE;
			if(ou.flags & UserVacation) nu.vacation = TRUE;
			if(ou.flags & UserRegExp) nu.regexp = TRUE;

			nu.number = ou.number;
			if(ou.fwd_mask) nu.bbs = TRUE;
			nu.message = ou.last_msg;
			nu.lines = ou.lines;
			nu.last_activity = ou.last_activity;

			strcpy(nu.email_addr, ou.uucp);
			strcpy(nu.freq, ou.freq);
			strcpy(nu.lname, ou.lname);
			strcpy(nu.phone, ou.phone);
			strcpy(nu.tnc, ou.tnc);
			strcpy(nu.computer, ou.computer);
			strcpy(nu.rig, ou.rig);
			strcpy(nu.software, ou.software);

			for(i=0; i<20; i++) {
				if(ou.include[i].str[0])
					sprintf(nu.include, "%s %s", nu.include, ou.include[i].str);
				if(ou.exclude[i].str[0])
					sprintf(nu.exclude, "%s %s", nu.exclude, ou.exclude[i].str);
			}

			for(i=0; i<10; i++)
				strcpy(nu.macro[i], ou.macro[i]);

			nu.connect[0].firstseen = ou.connect[0].firstseen;
			nu.connect[0].lastseen = ou.connect[0].lastseen;
			nu.connect[0].count = ou.connect[0].count;
			if(ou.allowed & UserOnTnc144)
				nu.connect[0].allow = TRUE;

			nu.connect[1].firstseen = ou.connect[1].firstseen;
			nu.connect[1].lastseen = ou.connect[1].lastseen;
			nu.connect[1].count = ou.connect[1].count;
			if(ou.allowed & UserOnTnc220)
				nu.connect[1].allow = TRUE;

			nu.connect[2].firstseen = ou.connect[2].firstseen;
			nu.connect[2].lastseen = ou.connect[2].lastseen;
			nu.connect[2].count = ou.connect[2].count;
			if(ou.allowed & UserOnTnc440)
				nu.connect[2].allow = TRUE;

			nu.connect[3].firstseen = ou.connect[4].firstseen;
			nu.connect[3].lastseen = ou.connect[4].lastseen;
			nu.connect[3].count = ou.connect[4].count;
			if(ou.allowed & UserOnPhoneSLOW)
				nu.connect[3].allow = TRUE;

			nu.connect[4].firstseen = ou.connect[3].firstseen;
			nu.connect[4].lastseen = ou.connect[3].lastseen;
			nu.connect[4].count = ou.connect[3].count;
			if(ou.allowed & UserOnPhoneFAST)
				nu.connect[4].allow = TRUE;

			nu.connect[5].firstseen = ou.connect[5].firstseen;
			nu.connect[5].lastseen = ou.connect[5].lastseen;
			nu.connect[5].count = ou.connect[5].count;
			if(ou.allowed & UserOnConsole)
				nu.connect[5].allow = TRUE;

			sprintf(fn, "%s/%s", NEWUSRPATH, dp->d_name);
			if((fp = fopen(fn, "w")) == NULL) {
				printf("problem opening %s\n", fn);
				continue;
			}

			fwrite(&nu, SizeOfUserInfo, 1, fp);
			fclose(fp);
		}

	closedir(dirp);
	return OK;
}
