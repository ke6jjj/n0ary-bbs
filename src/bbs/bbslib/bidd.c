#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"

/* if we have trouble connecting to the server then just accept the
 * message. Better too many than miss the important one.
 */

static int Bidd_Port = 0;
static int bidd_sock = ERROR;

static int
bidd_read(char *buf, int len)
{
	switch(socket_read_line(bidd_sock, buf, len, 60)) {
	case sockOK:
		break;
	case sockMAXLEN:
		log_error("bidd_read: line longer than expected");
		return ERROR;
	case sockERROR:
	case sockTIMEOUT:
		log_error("bidd_read: error reading from socket");
	default:
		return ERROR;
	}
	return OK;
}

int
bidd_open(void)
{
	char buf[80];

	if(bidd_sock != ERROR)
		return OK;

	if(Bidd_Port == 0)
		Bidd_Port = atoi(bbsd_get_variable("BIDD_PORT"));
	
	if((bidd_sock = socket_open(Bbs_Host, Bidd_Port)) == ERROR)
		exit(200);

	if(bidd_read(buf, 80) != OK)
		exit(200);

	return OK;
}

void
bidd_close(void)
{
	if(bidd_sock != ERROR)
		return;
	close(bidd_sock);
	bidd_sock = ERROR;
}

int
bid_chk(char *bid)
{
	static char buf[80];

	if(bidd_open() != OK)
		return ERROR;

	uppercase(bid);
	sprintf(buf, "%s\n", bid);
	socket_raw_write(bidd_sock, buf);

	if(bidd_read(buf, 80) != OK)
		exit(200);

	bidd_close();

	if(buf[0] == 'O' && buf[1] == 'K')
		return OK;
	return TRUE;
}

int
bid_add(char *bid)
{
	static char buf[80];

	if(bidd_open() != OK)
		return ERROR;

	uppercase(bid);
	sprintf(buf, "ADD %s\n", bid);
	socket_raw_write(bidd_sock, buf);

	if(bidd_read(buf, 80) != OK)
		exit(200);

	bidd_close();

	if(buf[0] == 'O' && buf[1] == 'K')
		return OK;
	return TRUE;
}

int
bid_delete(char *bid)
{
	static char buf[80];

	if(bidd_open() != OK)
		return ERROR;

	uppercase(bid);
	sprintf(buf, "DELETE %s\n", bid);
	socket_raw_write(bidd_sock, buf);

	if(bidd_read(buf, 80) != OK)
		exit(200);

	bidd_close();

	if(buf[0] == 'O' && buf[1] == 'K')
		return OK;
	return TRUE;
}

int
mid_chk(char *bid)
{
	static char buf[80];

	if(bidd_open() != OK)
		return ERROR;

	uppercase(bid);
	sprintf(buf, "MID %s\n", bid);
	socket_raw_write(bidd_sock, buf);

	if(bidd_read(buf, 80) != OK)
		exit(200);

	bidd_close();

	if(buf[0] == 'O' && buf[1] == 'K')
		return OK;
	return TRUE;
}

