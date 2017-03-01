#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <termios.h>

#include "c_cmmn.h"

static long
get_peer_address(int fd)
{
	struct sockaddr_in a;
	int y = sizeof(a);

	if(getpeername(fd, (struct sockaddr *)&a, &y) < 0)
		return ERROR;

#ifdef S_un
	printf("peer: [0x%x] %s\n", a.sin_addr.S_un.S_addr, inet_ntoa(a.sin_addr));
	return(a.sin_addr.S_un.S_addr);
#else
	printf("peer: [0x%x] %s\n", a.sin_addr.s_addr, inet_ntoa(a.sin_addr));
	return(a.sin_addr.s_addr);
#endif
}

static long
get_our_address(int fd)
{
	struct sockaddr_in a;
	int y = sizeof(a);

	if(getsockname(fd, (struct sockaddr *)&a, &y) < 0)
		return ERROR;

#ifdef S_un
	printf("us: [0x%x] %s\n", a.sin_addr.S_un.S_addr, inet_ntoa(a.sin_addr));
	return(a.sin_addr.S_un.S_addr);
#else
 	printf("us: [0x%x] %s\n", a.sin_addr.s_addr, inet_ntoa(a.sin_addr));
 	return(a.sin_addr.s_addr);
#endif
}

int
peer_in_our_net(int fd, long mask)
{
	long peer = get_peer_address(fd) & mask;
	long us = get_our_address(fd) & mask;

	return (peer == us);
}
