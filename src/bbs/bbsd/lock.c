#include <stdio.h>
#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bbsd.h"

struct Ports *PortList = NULL;


void
lock_clear_all(void)
{
	struct Ports *port = PortList;

	while(port) {
		port->lock = FALSE;
		port->reason = NULL;
		NEXT(port);
	}
}

void
lock_all(char *reason)
{
	struct Ports *port = PortList;

	while(port) {
		if(port->lockable) {
			port->lock = TRUE;
			port->reason = reason;
		}
		NEXT(port);
	}
}

struct Ports *
locate_port(char *via)
{
	struct Ports *port = PortList;

	while(port) {
		if(!stricmp(port->name, via))
			return port;
		NEXT(port);
	}
	return NULL;
}
