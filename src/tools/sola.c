#include <stdio.h>

#include "c_cmmn.h"
#include "tools.h"
#include "sola.h"

int
read_sola(int *bv, int *iv, int *ov)
{
	char buf[80];
	char *c = buf;
	int sock = ERROR;
	int up;

	if((sock = socket_open(SOLA_HOST, SOLA_PORT)) == ERROR)
		exit(1);

	while(read(sock, c, 1) > 0)
		c++;
	close(sock);

	/* LINE|BATTERY in out bat */

	c = buf;

	switch(*c) {
	case 'L':
		up = TRUE;
		break;
	case 'B':
		up = FALSE;
		break;
	default:
		return ERROR;
	}

	while(*c != ' ')
		c++;
	sscanf(c, "%d %d %d", iv, ov, bv);
	return up;
}
