#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bbslib.h"

static int parse_local_addr(const char *addrspec, struct RemoteAddr *);
static int parse_ax25_addr(const char *addrspec, struct RemoteAddr *);
static int parse_tcpipv4_addr(const char *addrspec, struct RemoteAddr *);
static int parse_tcpipv6_addr(const char *addrspec, struct RemoteAddr *);
static int ntoa_tcpipv4(struct in_addr pin, int port, struct RemoteAddr *);
static int ntoa_tcpipv6(struct in6_addr pin, int port, struct RemoteAddr *);

int
parse_remote_addr(const char *addrspec, struct RemoteAddr *addr)
{
	char protostr[10];
	const char *colon, *rest;

	colon = index(addrspec, ':');
	if (colon == NULL)
		return -1;

	if (colon - addrspec > sizeof(protostr) - 1)
		return -2;

	memcpy(protostr, addrspec, colon - addrspec);
	protostr[colon - addrspec] = '\0';
	rest = colon + 1;

	if (strcasecmp(protostr, "local") == 0) {
		return parse_local_addr(rest, addr);
	} else if (strcasecmp(protostr, "ax25") == 0) {
		return parse_ax25_addr(rest, addr);
	} else if (strcasecmp(protostr, "tcpip") == 0) {
		return parse_tcpipv4_addr(rest, addr);
	} else if (strcasecmp(protostr, "tcpipv6") == 0) {
		return parse_tcpipv6_addr(rest, addr);
	}

	return -3;
}

int
print_remote_addr(const struct RemoteAddr *addr, char *str, size_t len)
{
	switch (addr->addr_type) {
	case pLOCAL:
		snprintf(str, len, "%s:", "local");
		return 0;
	case pAX25:
		if (addr->u.ax25.ssid == 0)
			snprintf(str, len, "ax25:%s", addr->u.ax25.callsign);
		else
			snprintf(str, len, "ax25:%s-%d", addr->u.ax25.callsign,
				addr->u.ax25.ssid);
		return 0;
	case pTCPIPv4:
		snprintf(str, len, "tcpip:%s:%d", addr->u.tcpipv4.ip,
			addr->u.tcpipv4.port);
		return 0;
	case pTCPIPv6:
		snprintf(str, len, "tcpipv6:%s:%d", addr->u.tcpipv6.ip,
			addr->u.tcpipv6.port);
		return 0;
	default:
		break;
	}

	snprintf(str, len, "unknown");
	return 0;
}

int
get_remote_addr(int sock, struct RemoteAddr *addr)
{
	int res;
	socklen_t len;
	struct sockaddr sa;
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;

	len = sizeof(sa);
	res = getpeername(sock, &sa, &len);
	if (res != 0)
		return -1;

	switch (sa.sa_family) {
	case AF_INET:
		sin = (struct sockaddr_in *) &sa;
		return ntoa_tcpipv4(sin->sin_addr, ntohs(sin->sin_port), addr);
	case AF_INET6:
		sin6 = (struct sockaddr_in6 *) &sa;
		return ntoa_tcpipv6(sin6->sin6_addr, ntohs(sin6->sin6_port),
			addr);
	default:
		break;
	}

	return -2;
}

static int
parse_local_addr(const char *addrspec, struct RemoteAddr *addr)
{
	addr->addr_type = pLOCAL;
	return 0;
}

/* callsign[-ssid] */
static int
parse_ax25_addr(const char *addrspec, struct RemoteAddr *addr)
{
	size_t lencall, i;
	long ssid;
	const char *dash;
	char *rest;

	dash = index(addrspec, '-');
	if (dash == NULL) {
		lencall = strlen(addrspec);
		ssid = 0;
	} else {
		ssid = strtol(dash + 1, &rest, 10);
		if (*rest != '\0') {
			/* Invalid character in SSID */
			return -4;
		}
		if (ssid > 15)
			return -5;
		lencall = dash - addrspec;
	}

	if (lencall > LenCALL)
		return -6;

	for (i = 0; i < lencall; i++) {
		if (!isalnum(addrspec[i])) {
			/* Invalid character in callsign */
			return -7;
		}
		addr->u.ax25.callsign[i] = toupper(addrspec[i]);
	}

	addr->addr_type = pAX25;
	addr->u.ax25.callsign[lencall] = '\0';
	addr->u.ax25.ssid = ssid;

	return 0;
}

static int
parse_tcpipv4_addr(const char *addrspec, struct RemoteAddr *addr)
{
	char ipaddr[INET_ADDRSTRLEN+1];
	const char *colon;
	char *rest;
	struct in_addr pin;
	long port;

	colon = index(addrspec, ':');
	if (colon == NULL)
		return -4;

	memcpy(ipaddr, addrspec, colon - addrspec);
	ipaddr[colon - addrspec] = '\0';

	if (inet_aton(ipaddr, &pin) != 1)
		return -5;

	port = strtol(colon + 1, &rest, 10);
	if (*rest != '\0')
		return -6;

	if (port < 0 || port > 65535)
		return -7;

	return ntoa_tcpipv4(pin, port, addr);
}

static int
parse_tcpipv6_addr(const char *addrspec, struct RemoteAddr *addr)
{
	char ip6addr[INET6_ADDRSTRLEN+1];
	const char *colon;
	char *rest;
	struct in6_addr pin;
	long port;

	/*
	 * IPv6 addresses in presentation format contain many colons. So to
	 * find the port number we must find the last colon, using rindex().
	 */
	colon = rindex(addrspec, ':');
	if (colon == NULL)
		return -4;

	memcpy(ip6addr, addrspec, colon - addrspec);
	ip6addr[colon - addrspec] = '\0';

	if (inet_pton(AF_INET6, ip6addr, &pin) != 1)
		return -5;

	port = strtol(colon + 1, &rest, 10);
	if (*rest != '\0')
		return -6;

	if (port < 0 || port > 65535)
		return -7;

	return ntoa_tcpipv6(pin, port, addr);
}

static int
ntoa_tcpipv4(struct in_addr pin, int port, struct RemoteAddr *addr)
{
	addr->addr_type = pTCPIPv4;
	addr->u.tcpipv4.port = port;
	inet_ntoa_r(pin, addr->u.tcpipv4.ip, sizeof(addr->u.tcpipv4.ip));

	return 0;
}

static int
ntoa_tcpipv6(struct in6_addr pin, int port, struct RemoteAddr *addr)
{
	addr->addr_type = pTCPIPv6;
	addr->u.tcpipv6.port = port;
	inet_ntop(AF_INET6, &pin, addr->u.tcpipv6.ip,
		sizeof(addr->u.tcpipv6.ip));

	return 0;
}
