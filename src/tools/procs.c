#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "tools.h"

struct active_procs *ActiveProcs = NULL;

struct active_procs *
proc_append(void)
{
	struct active_procs
		*tap = ActiveProcs,
		*ap = malloc_struct(active_procs);

	if(tap == NULL)
		ActiveProcs = ap;
	else {
		while(tap->next)
			NEXT(tap);
		tap->next = ap;
	}
	ap->fd = ERROR;
	return ap;
}

struct active_procs *
proc_prepend(void)
{
	struct active_procs
		*ap = malloc_struct(active_procs);

	ap->next = ActiveProcs;
	ActiveProcs = ap;
	ap->fd = ERROR;
	return ap;
}

struct active_procs *
proc_remove(struct active_procs *ap)
{
	struct active_procs *tap = ActiveProcs;

	if((long)ActiveProcs == (long)ap)
		NEXT(ActiveProcs);
	else {
		while(tap) {
			if((long)tap->next == (long)ap) {
				tap->next = tap->next->next;
				break;
			}
			NEXT(tap);
		}
	}

	if(ap->fd != ERROR)
		close(ap->fd);
	free(ap);
	return tap;
}
