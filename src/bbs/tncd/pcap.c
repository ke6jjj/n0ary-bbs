#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alib.h"
#include "kiss.h"
#include "tools.h"
#include "pcap.h"

struct pcap_logging {
	int       reopen_signal;
	int       registered;
	char     *pcap_path;
	kiss     *dev;

	alEventHandle ev;
};

static int setup_logging(const char *path, kiss *dev);
static void pcap_logging_signal_recvd(void *obj, void *arg0, int arg1);

pcap_logging *
pcap_logging_new(int signo, const char *path, kiss *dev)
{
	pcap_logging *pcap = alMalloc(pcap_logging, 1);
	if (pcap == NULL)
		return NULL;

	if ((pcap->pcap_path = strdup(path)) == NULL)
		goto PathDupFailed;

	pcap->reopen_signal = signo;
	pcap->registered = 0;
	pcap->dev = dev;

	return pcap;

PathDupFailed:
	free(pcap);
	return NULL;
}

int
pcap_logging_start(pcap_logging *pcap)
{
	int res;
	alCallback cb;
	struct sigaction sa;

	if (setup_logging(pcap->pcap_path, pcap->dev) != 0)
		goto FailedToOpenLog;

	/*
	 * Ignore the designated signal so that it doesn't cause the process
	 * to exit.
	 * This does not affect the ability to receive the signal via alib.
	 */
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	res = sigaction(pcap->reopen_signal, &sa, NULL);
	if (res == -1 ) {
		log_error("unable to set reopen signal to 'ignore'");
		goto SigIgnoreFailed;
	}

	AL_CALLBACK(&cb, pcap, pcap_logging_signal_recvd);
	res = alEvent_registerSignal(pcap->reopen_signal, ALSIG_SIGNALED, cb,
		&pcap->ev);
	if (res != 0) {
		log_error("unable to register pcap rotate signal with alib");
		goto AlibRegisterFailed;
	}

	pcap->registered = 1;

	return 0;

AlibRegisterFailed:
	sa.sa_handler = SIG_DFL;
	sa.sa_flags = 0;
	sigaction(pcap->reopen_signal, &sa, NULL);
SigIgnoreFailed:
	setup_logging(NULL, pcap->dev);
FailedToOpenLog:
	return -1;
}

static int
setup_logging(const char *path, kiss *dev)
{
	FILE *pcap;

	if (path != NULL) {
		pcap = fopen(path, "ab");
		if (pcap == NULL) {
			log_error("couldn't setup PCAP dumping to "
				"'%s'.", path);
			return -1;
		}
	} else {
		pcap = NULL;
	}

	if (kiss_set_pcap_dump(dev, pcap) != 0) {
		log_error("couldn't ask KISS to pcap dump");
		return -1;
	}

	return 0;
}

static void
pcap_logging_signal_recvd(void *obj, void *arg0, int arg1)
{
	pcap_logging *pcap;

	pcap = obj;

	log_info("Re-opening pcap log file '%s'.", pcap->pcap_path);

	(void) setup_logging(pcap->pcap_path, pcap->dev);
}

void
pcap_logging_stop(pcap_logging *pcap)
{
	struct sigaction sa;

	if (pcap->registered != 0) {
		alEvent_deregister(pcap->ev);
		setup_logging(NULL, pcap->dev);
		sa.sa_handler = SIG_DFL;
		sa.sa_flags = 0;
		sigaction(pcap->reopen_signal, &sa, NULL);
		pcap->registered = 0;
	}
}

void
pcap_logging_free(pcap_logging *pcap)
{
	free(pcap->pcap_path);
	free(pcap);
}
