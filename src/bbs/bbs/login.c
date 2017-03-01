#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/file.h>
#include <errno.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "function.h"
#include "user.h"
#include "vars.h"
#include "wp.h"
#include "callbk.h"
#include "tokens.h"
#include "message.h"
#include "maintenance.h"

static void
	welcome_new_user_in_callbook(void),
	welcome_new_user_in_wp(void),
	welcome_new_user(void),
	welcome_old_user(void);

time_t login_time;
time_t logout_time;
char *login_method;
long login_method_num;

int
logout_user(void)
{
	char buf[256];
	int fd;
	int min, sec;
	int result = ERROR;

	logout_time = Time(NULL);
	sec = logout_time - login_time;
	min = sec/60;
	sec %= 60;

	sprintf(buf, "%"PRTMd" %"PRTMd" %s %ld\t; %s for %d:%02d on %s",
		login_time, logout_time,
		Via,
		user_get_value(uNUMBER),
		usercall,
		min, sec,
		ctime(&login_time));

	if((fd = open(Bbs_History_File, O_WRONLY|O_APPEND|O_CREAT, 0666)) > 0) {

#ifdef F_LOCK
		if(lockf(fd, F_LOCK, 0) == 0) {
			lseek(fd, 0, 2);
			write(fd, buf, strlen(buf));
			lockf(fd, F_ULOCK, 0);
			result = OK;
		} else
			error_log("logout_user.lockf(): %s", sys_errlist[errno]);
		close(fd);
#else
		if(flock(fd, LOCK_EX) == 0) {
			lseek(fd, 0, 2);
			write(fd, buf, strlen(buf));
			flock(fd, LOCK_UN);
			result = OK;
		} else
			error_log("logout_user.flock(): %s", sys_errlist[errno]);
		close(fd);
#endif
	} else
		error_log("logout_user.open(%s): %s",
			Bbs_History_File, sys_errlist[errno]);
	return result;
}

int
login_user(void)
{
	if(user_open() == ERROR) {		/* new user */
		user_create();

		if(ImNonHam)
			welcome_new_user();
		else {
			struct callbook_entry cb;
			int not_in_book = lookup_call_callbk(usercall, &cb);
			int not_in_wp = wp_cmd(usercall);

			if(not_in_wp) {
				wp_create_user(usercall);
				if(not_in_book)
					welcome_new_user();
				else {
					char buf[80];
					case_string(cb.fname, CapFirst);
					case_string(cb.lname, CapFirst);
					case_string(cb.city, CapFirst);
					case_string(cb.state, AllUpperCase);
					sprintf(buf, "%s, %s", cb.city, cb.state);

					wp_set_field(usercall, wQTH, wGUESS, buf);
					wp_set_field(usercall, wZIP, wGUESS, cb.zip);
					wp_set_field(usercall, wFNAME, wGUESS, cb.fname);
					user_set_field(uLNAME, cb.lname);

					welcome_new_user_in_callbook();
				}
			} else {	/* in wp */
				if(not_in_book)
					welcome_new_user_in_wp();
				else {
					char *p;

					p = wp_get_field(usercall, wFNAME);
					if(*p == '?') {
						case_string(cb.fname, CapFirst);
						wp_set_field(usercall, wFNAME, wUSER, cb.fname);
					}

					p = wp_get_field(usercall, wQTH);
					if(*p == '?') {
						char buf[80];
						case_string(cb.fname, CapFirst);
						case_string(cb.lname, CapFirst);
						case_string(cb.city, CapFirst);
						case_string(cb.state, AllUpperCase);
						sprintf(buf, "%s, %s", cb.city, cb.state);
						wp_set_field(usercall, wQTH, wUSER, buf);
					}

					p = wp_get_field(usercall, wZIP);
					if(*p == '?')
						wp_set_field(usercall, wZIP, wUSER, cb.zip);

					case_string(cb.lname, CapFirst);
					user_set_field(uLNAME, cb.lname);
					welcome_new_user_in_callbook();
				}
			}
		}
	} else {
		user_refresh();
		if(wp_cmd(usercall) < 0)
			if(wp_create_user(usercall) < 0)
				return ERROR;
		welcome_old_user();
	}

	ImLogging = user_check_flag(uLOG);
	if(user_allowed_on_port(Via) == FALSE)
		return error(76);

	wp_seen(usercall);
	user_login(Via);
	login_time = Time(NULL);

	if(port_type(Via) == tPHONE && ImHalfDuplex)
		if(echo_off() == ERROR)
			return ERROR;

	if(!ImPotentialBBS && !batch_mode)
		fill_in_blanks();

	return OK;
}


static void
welcome_new_user_in_callbook(void)
{
	PRINTF("%s\n", build_sid());
	system_msg(50);
	system_msg_string(51, wp_get_field(usercall, wQTH));
	system_msg(58);
	system_msg(57);
}

static void
welcome_new_user_in_wp(void)
{
	PRINTF("%s\n", build_sid());
	system_msg(50);
	system_msg_string(52, wp_get_field(usercall, wQTH));
	system_msg(58);
	system_msg(57);
}

static void
welcome_new_user(void)
{
	char buf[80];

	PRINTF("%s\n", build_sid());
	system_msg(50);
	system_msg(53);
	system_msg(54);
	GETnSTR(buf, LenFNAME, CapFirst);
	wp_set_field(usercall, wFNAME, wUSER, buf);
	system_msg(55);
	GETnSTR(buf, LenLNAME, CapFirst);
	user_set_field(uLNAME, buf);
	PRINT("\n");
	system_msg(57);
}

static void
welcome_old_user(void)
{
	PRINTF("%s\n", build_sid());
	if(!ImPotentialBBS) {
		system_msg(50);
		system_msg(58);
		system_msg_string(59, wp_get_field(usercall, wHOME));
		if(AutoVacation)
			PRINT("\nYOU HAVE VACATION MODE TURNED ON\n");
	}
}
