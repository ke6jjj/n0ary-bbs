#include <unistd.h>

#include "c_cmmn.h"
#include "tools.h"
#include "dectalk.h"

int
talk(char *str)
{
	int sock;
	if((sock = socket_open(DECTALK_HOST, DECTALK_PORT)) == ERROR)
		return ERROR;
	write(sock, str, strlen(str));
	close(sock);
	return OK;
}
