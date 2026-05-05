#ifndef PCAP_LOGGING_H
#define PCAP_LOGGING_H

#include "kiss.h"

/*
 * Allows PCAP log file re-opening to facilitate log rotation on receipt
 * of SIGHUP.
 */
typedef struct pcap_logging pcap_logging;

pcap_logging *pcap_logging_new(int signo, const char *path, kiss *dev);
int pcap_logging_start(pcap_logging *pcap_r);
void pcap_logging_stop(pcap_logging *pcap_r);
void pcap_logging_free(pcap_logging *pcap_r);

#endif
