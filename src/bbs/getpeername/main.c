#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	struct sockaddr sa;
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;
	socklen_t salen;
	int res;
	char buf[1024];

	salen = sizeof(sa);
	res = getpeername(STDIN_FILENO, &sa, &salen);
	if (res != 0)
		return 1;

	switch (sa.sa_family) {
	case AF_INET:
		sin = (struct sockaddr_in *)&sa;
		printf("inet %s %d\n", inet_ntoa(sin->sin_addr),
			ntohs(sin->sin_port));
		return 0;
	case AF_INET6:
		sin6 = (struct sockaddr_in6 *)&sa;
		inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf));
		printf("inet6 %s %d\n", buf, ntohs(sin6->sin6_port));
		return 0;
	default:
		printf("%d\n", sa.sa_family);
		return 1;
	}
}
