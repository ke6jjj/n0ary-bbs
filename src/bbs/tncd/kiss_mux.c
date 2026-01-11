#include <sys/queue.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "alib.h"
#include "bsd.h"
#include "c_cmmn.h"
#include "kiss_mux.h"
#include "socket.h"
#include "tools.h"

struct kiss_session {
	LIST_ENTRY(kiss_session) ks_link;
	asy *ks_asy;
	slip *ks_slip;
	int ks_fd;
};

static int kiss_socket = ERROR;
static int see_others = TRUE;
static alEventHandle kiss_event_handle;
static slip *master_slip;
static kiss *master_kiss;

LIST_HEAD(,kiss_session) kiss_sessions;

static void kiss_mux_accept(void *obj, void *arg0, int arg1);
static void kiss_mux_deliver(struct mbuf *bp, struct kiss_session *sender);
static void kiss_session_disc(struct kiss_session *ks);
static void kiss_session_asy_notification(void *, asy *, int, int);
static int  kiss_session_slip_recv(void *obj, struct mbuf *bp);

static struct mbuf *mbuf_dup(const struct mbuf *src, size_t len);

/*
 * Handle a KISS packet that has been received from the real TNC.
 * The incoming packet comes from the SLIP layer and will have a KISS
 * header on it.
 */
int
kiss_nexus_recv(void *up_kiss, struct mbuf *bp)
{
	/* Deliver to all subscribed KISS sessions */
	kiss_mux_deliver(bp, /* Whom to skip */ NULL);

	/* Now deliver the packet to the TNCD network stack. */
	return kiss_recv(up_kiss, bp);
}

/*
 * Handle a KISS packet that is destined for the real TNC.
 */
int
kiss_nexus_send(void *down_slip, struct mbuf *bp)
{
	if (see_others)
		/* Deliver to all subscribed KISS sessions */
		kiss_mux_deliver(bp, /* Whom to skip */ NULL);

	/* Now deliver the packet down to the real TNC network stack. */
	return slip_output(down_slip, bp);
}

int
kiss_mux_init(kiss *kiss, slip *slip, int others, char *m_bindaddr, int m_port)
{
	alCallback cb;

	LIST_INIT(&kiss_sessions);
	master_kiss = kiss;
	master_slip = slip;
	see_others = others;

	if (m_port == 0)
		return OK;

	if((kiss_socket = socket_listen(m_bindaddr, &m_port)) < 0) {
		log_error(
			"kiss_mux_init: Problem opening mux socket "
			"... aborting");
		return ERROR;
	}

	AL_CALLBACK(&cb, NULL, kiss_mux_accept);
	if (alEvent_registerFd(kiss_socket, ALFD_READ, cb,
		&kiss_event_handle) != 0)
	{
		close(kiss_socket);
		kiss_socket = ERROR;
		log_error("problem registering kiss mux socket");
		return ERROR;
	}

	return OK;
}

int
kiss_mux_shutdown(void)
{
	struct kiss_session *ks, *kstmp;

	LIST_FOREACH_SAFE(ks, &kiss_sessions, ks_link, kstmp) {
		kiss_session_disc(ks);
	}
	if (kiss_event_handle != NULL) {
		alEvent_deregister(kiss_event_handle);
		kiss_event_handle = NULL;
	}
	if (kiss_socket != ERROR) {
		close(kiss_socket);
		kiss_socket = ERROR;
	}

	return 0;
}

static void
kiss_mux_deliver(struct mbuf *bp, struct kiss_session *sender)
{
	struct kiss_session *ks;

	if (!LIST_EMPTY(&kiss_sessions)) {
		size_t len = len_mbuf(bp);
		LIST_FOREACH(ks, &kiss_sessions, ks_link) {
			if (ks == sender)
				continue;
			struct mbuf *dup = mbuf_dup(bp, len);
			if (dup != NULL) {
				slip_output(ks->ks_slip, dup);
			}
		}
	}
}

static void
kiss_mux_accept(void *obj, void *arg0, int arg1)
{
	int fd, res;
	alCallback cb;
	struct kiss_session *ks;

	if((fd = accept_socket(kiss_socket)) == ERROR) {
		log_error("kiss mux accept() error");
		goto AcceptError;
	}

	if ((ks = malloc_struct(kiss_session)) == NULL) {
		log_error("kiss mux session alloc error");
		goto AllocError;
	}

	ks->ks_fd = fd;

	if ((ks->ks_asy = asy_init_from_fd(ks->ks_fd)) == NULL) {
		log_error("kiss mux async alloc failed");
		goto AsyncAllocError;
	}

	if ((ks->ks_slip = slip_init(0)) == NULL) {
		log_error("kiss mux slip alloc failed");
		goto SlipAllocError;
	}

	/* Have the asy object send its data to the SLIP decoder */
	asy_set_recv(ks->ks_asy, slip_input, ks->ks_slip);
	/* Have the SLIP decoder send its packets to the session */
	slip_set_recv(ks->ks_slip, kiss_session_slip_recv, ks);
	
	/* Have the SLIP encoder send its packets to the asy object */
	slip_set_send(ks->ks_slip, asy_send, ks->ks_asy);
	/* Have the asy notify the session if there are problems */
	asy_set_notify(ks->ks_asy, kiss_session_asy_notification, ks);

	LIST_INSERT_HEAD(&kiss_sessions, ks, ks_link);

	asy_start(ks->ks_asy);

	return;

RegisterError:
	slip_deinit(ks->ks_slip);
SlipAllocError:
	asy_deinit(ks->ks_asy);
AsyncAllocError:
	free(ks);
AllocError:
	close(fd);
AcceptError:
	return;
}

static int
kiss_session_slip_recv(void *obj, struct mbuf *bp)
{
	struct kiss_session *ks = obj;
	struct mbuf *to_host;

	if (see_others) {
		/* Deliver to all other subscribed KISS sessions */
		kiss_mux_deliver(bp, /* Whom to skip */ ks);

		/* Deliver the packet up to the TNCD network stack. */
		size_t len = len_mbuf(bp);
		if ((to_host = mbuf_dup(bp, len)) != NULL)
			kiss_recv(master_kiss, to_host);
	}

	/* Deliver packet down to the real TNC */
	return slip_output(master_slip, bp);
}

static void
kiss_session_asy_notification(void *obj, asy *sender, int is_read, int error)
{
	struct kiss_session *ks = obj;
	assert(sender == ks->ks_asy);

	/* Something has gone wrong. Shut this session down */
	kiss_session_disc(ks);
}

static void
kiss_session_disc(struct kiss_session *ks)
{
	LIST_REMOVE(ks, ks_link);

	if (ks->ks_asy != NULL) {
		asy_stop(ks->ks_asy);
		asy_deinit(ks->ks_asy);
	}
	if (ks->ks_slip != NULL) {
		slip_deinit(ks->ks_slip);
	}
	close(ks->ks_fd);
	free(ks);
}

static struct mbuf *
mbuf_dup(const struct mbuf *src, size_t len)
{
	struct mbuf *cp;
	size_t cnt, amt;

	if ((cp = alloc_mbuf(len)) == NULL)
		return NULL;

	for (cnt = 0; src != NULL && len > 0;) {
		amt = len > src->cnt ? src->cnt : len;
		memcpy(&cp->data[cnt], src->data, amt);
		len -= amt;
		cnt += amt;
		src = src->next;
	}

	cp->cnt = cnt;

	if (len != 0) {
		free_p(cp);
		return NULL;
	}

	return cp;
}
