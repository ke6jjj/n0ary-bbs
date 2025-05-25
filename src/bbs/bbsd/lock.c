#include <stdio.h>
#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bbsd.h"

struct port_list PortList = SLIST_HEAD_INITIALIZER(PortList);

void
lock_clear_all(void)
{
	struct Port *port;

	SLIST_FOREACH(port, &PortList, entries) {
		port->lock = FALSE;
		port->reason = NULL;
	}
}

void
lock_all(char *reason)
{
	struct Port *port;

	SLIST_FOREACH(port, &PortList, entries) {
		if(port->lockable) {
			port->lock = TRUE;
			port->reason = reason;
		}
	}
}

struct Port *
locate_port(char *via)
{
	struct Port *port;

	SLIST_FOREACH(port, &PortList, entries) {
		if(!stricmp(port->name, via))
			return port;
	}
	return NULL;
}
