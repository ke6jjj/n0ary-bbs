#include <stdio.h>
#include <fcntl.h>
#include <sys/termios.h>

#include "c_cmmn.h"
#include "config.h"

#define DEVICE	"/dev/ttyr1"
#define TIMEOUT	40	/* 20 minutes */

static char incoming[1024];

open_phone(phone_number)
char *phone_number;
{
	struct termios tt;
	int modem;
	int error_on_hill = 5;
	
	while(error_on_hill--) {
		int wait_for_line = TIMEOUT;

		while((modem = open(DEVICE, O_RDWR)) < 0) {
			if(wait_for_line-- == 0) {
				wx_log("retries exhausted, waiting for telephone to become available");
				return ERROR;
			}
			wx_log("busy ");
			sleep(30);
		}

		if(wait_for_line != TIMEOUT) {
			char buf[80];
			sprintf(buf, "Line busy for %d tries, %d minutes", TIMEOUT - wait_for_line,
				(TIMEOUT - wait_for_line)/2);
			wx_log(buf);
		}

		if(dial_out(modem, phone_number) == OK)
			return modem;

		close(modem);
		sleep(10);
	}

	wx_log("retries exhausted, problems with phone on hill");
	return ERROR;
}

dial_out(modem, phone_number)
int modem;
char *phone_number;
{
	char buf[80];
	int result;

	write(modem, "ATV0E0X0\n", 9);
	result = get_result(modem);
	if(result != 0) {
		char buf[80];
		sprintf(buf, "issued ATV0E0X0 expected 0 but got %d", result);
		wx_log(buf);
		return ERROR;
	}

	sprintf(buf, "ATDT%s\n", phone_number);
	write(modem, buf, strlen(buf));

	result = get_result(modem);
	if(result != 1) {
		char buf[80];
		sprintf(buf, "issued ATDT expected 1 but got %d [%s]", result, incoming);
		wx_log(buf);
		return ERROR;
	}

	sleep(1);
	return OK;
}

/* a result is a single digit followed by a NEWLINE, anything else
 * is an error.
 */

get_result(fd)
int fd;
{
	char *p = incoming;
	int len = 0;

	do {
		if(len > 100)
			return ERROR;

		read(fd, &incoming[len], 1);
	} while(incoming[len++] != '\n');
	len--;
	incoming[len--] = 0;

	if(!isdigit(incoming[len])) {
		char buf[256];
		sprintf(buf, 
			"get_result: first character should have been a digit [%s]",
			incoming);
		wx_log(buf);
		return ERROR;
	}

	return incoming[len] - '0';
}

