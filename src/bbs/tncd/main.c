#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>

#include "alib.h"
#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "monitor.h"
#include "timer.h"
#include "ax25.h"
#include "tnc.h"
#include "slip.h"
#include "bsd.h"
#include "beacon.h"
#include "kiss_mux.h"

#include "ax_mbx.h"

char
    versionm[80],
    versionc[80],
    *Bbs_Call,
    *Bbs_My_Call,
    *Bbs_Unlock_Call,

    *Bbs_Dir,
    *Bin_Dir,
    *Default_Beacon_Call,
    *Default_Beacon_Dest,
    *Default_Beacon_Message,
    *Tncd_Host,
    *Tncd_Beacon_Call,
    *Tncd_Beacon_Dest,
    *Tncd_Beacon_Message,
    *Tncd_Control_Bind_Addr = NULL,
    *Tncd_Monitor_Bind_Addr = NULL,
    *Tncd_KISS_Mux_Bind_Addr = NULL,
    *Tncd_Device,
    *Tncd_Line_Protocol;

int
    Tncd_Control_Port,
    Tncd_Monitor_Port,
    Tncd_KISS_Mux_Port,
    Tncd_KISS_Mux_See_Others = 1,
    Tncd_Maxframe,
    Tncd_N2,
    Tncd_Paclen,
    Tncd_Pthresh,
    Tncd_Beacon_Interval,
    Tncd_SLIP_Flags,
    Default_Beacon_Interval;

int
    bbsd_sock,
    Tncd_T1init,
    Tncd_T2init,
    Tncd_T3init;

int background = 0;
int do_shutdown = FALSE;

static alEventHandle bbsd_ev;
static asy *Master_ASY;

struct ConfigurationList ConfigList[] = {
  { "BBS_MYCALL",                 tSTRING,    (int*)&Bbs_My_Call },
  { "BBS_UNLOCKCALL",             tSTRING,    (int*)&Bbs_Unlock_Call },
  { "BBS_HOST",                   tSTRING,    (int*)&Bbs_Host },
  { "BBS_DIR",                    tSTRING,    (int*)&Bbs_Dir },
  { "BBS_CALL",                   tSTRING,    (int*)&Bbs_Call },
  { "BIN_DIR",                    tDIRECTORY, (int*)&Bin_Dir },
  { "BBSD_PORT",                  tINT,       (int*)&Bbsd_Port },
  { "AX25_BEACON_INTERVAL",       tTIME,      (int*)&Default_Beacon_Interval },
  { "AX25_BEACON_MESSAGE",        tSTRING,    (int*)&Default_Beacon_Message },
  { "AX25_BEACON_CALL",           tSTRING,    (int*)&Default_Beacon_Call },
  { "AX25_BEACON_DEST",           tSTRING,    (int*)&Default_Beacon_Dest },
  { NULL,                         tEND,       NULL}
};

struct ConfigurationList DynamicConfigList[] = {
  { "",                                                       tCOMMENT, NULL },
  { " Prefix each entry name below with \"TNCx_\" (where x)", tCOMMENT, NULL },
  { " is a valid TNC port, ie. TNC2_DEVICE.",                 tCOMMENT, NULL },
  { "",                                                       tCOMMENT, NULL },
  { "CONTROL_PORT",               tINT,       (int*)&Tncd_Control_Port },
  { "CONTROL_BIND_ADDR",          tSTRING,    (int*)&Tncd_Control_Bind_Addr },
  { "MONITOR_PORT",               tINT,       (int*)&Tncd_Monitor_Port },
  { "MONITOR_BIND_ADDR",          tSTRING,    (int*)&Tncd_Monitor_Bind_Addr },
  { "DEVICE",                     tSTRING,    (int*)&Tncd_Device },
  { "LINE_PROTOCOL",              tSTRING,    (int*)&Tncd_Line_Protocol },
  { "",                                                       tCOMMENT, NULL },
  { "  AX25 KISS parameters",                                 tCOMMENT, NULL },
  { "",                                                       tCOMMENT, NULL },
  { "KISS_MUX_BIND_ADDR",         tSTRING,    (int*)&Tncd_KISS_Mux_Bind_Addr },
  { "KISS_MUX_PORT",              tINT,       (int*)&Tncd_KISS_Mux_Port },
  { "T1INIT",                     tTIME,      (int*)&Tncd_T1init },
  { "T2INIT",                     tTIME,      (int*)&Tncd_T2init },
  { "T3INIT",                     tTIME,      (int*)&Tncd_T3init },
  { "MAXFRAME",                   tINT,       (int*)&Tncd_Maxframe },
  { "N2",                         tINT,       (int*)&Tncd_N2 },
  { "PACLEN",                     tINT,       (int*)&Tncd_Paclen },
  { "PTHRESH",                    tINT,       (int*)&Tncd_Pthresh },
  { "FLAGS",                      tINT,       (int*)&Tncd_SLIP_Flags },
  { "BEACON_INTERVAL",            tTIME,      (int*)&Tncd_Beacon_Interval },
  { "BEACON_CALL",                tSTRING,    (int*)&Tncd_Beacon_Call },
  { "BEACON_DEST",                tSTRING,    (int*)&Tncd_Beacon_Dest },
  { "BEACON_MESSAGE",             tSTRING,    (int*)&Tncd_Beacon_Message },
  { "KISS_MUX_SEE_OTHERS",        tINT,       (int*)&Tncd_KISS_Mux_See_Others },
  { NULL,                         tEND,       NULL}
};

static void
preload(char *name)
{
	int i = 0;
	struct ax25_params *tax = tnc_ax25(name);
	struct ConfigurationList dynconfig[2], *cfg;
	char varname[256];

	/*
	 * First, load options through the classic configuration method.
	 */
	Tncd_Control_Bind_Addr = tnc_control_bind_addr(name);
	Tncd_Control_Port = tnc_port(name);
	Tncd_Monitor_Bind_Addr = tnc_monitor_bind_addr(name);
	Tncd_Monitor_Port = tnc_monitor_port(name);
	Tncd_Device = tnc_device(name);
        Tncd_Line_Protocol = tnc_line_protocol(name);
	Tncd_Name = name;
	Tncd_Host = tnc_host(name);

	Tncd_T1init = tax->t1;
	Tncd_T2init = tax->t2;
	Tncd_T3init = tax->t3;
	Tncd_Maxframe = tax->maxframe;
	Tncd_N2 = tax->n2;
	Tncd_Pthresh = tax->pthresh;
	Tncd_Paclen = tax->paclen;
	Tncd_SLIP_Flags = tax->flags;

	/*
	 * Next, load options via the new configuration method, allowing them
	 * to override options set via the classic method.
	 */
	dynconfig[1].token = NULL;
	dynconfig[1].type = 0;
	dynconfig[1].ptr = NULL;

	for(i=0; DynamicConfigList[i].token != NULL; i++) {
		cfg = &DynamicConfigList[i];
		if (cfg->type == tCOMMENT)
			continue;
#ifdef SUNOS
		sprintf(varname, "%s_%s", name, cfg->token);
#else
		snprintf(varname, sizeof(varname), "%s_%s", name, cfg->token);
#endif
		dynconfig[0].token = varname;
		dynconfig[0].type = cfg->type;
		dynconfig[0].ptr = cfg->ptr;

		bbsd_get_configuration(dynconfig);
	}

	/* Have the beaconing defaults trickle through */

	if (Default_Beacon_Call == NULL)
		Default_Beacon_Call = Bbs_My_Call;
	if (Default_Beacon_Dest == NULL)
		Default_Beacon_Dest = "ID";
	if (Default_Beacon_Message == NULL)
		Default_Beacon_Message = "";

	if (Tncd_Beacon_Interval == -1)
		Tncd_Beacon_Interval = Default_Beacon_Interval;

	if (Tncd_Beacon_Call == NULL)
		Tncd_Beacon_Call = Default_Beacon_Call;
	if (Tncd_Beacon_Dest == NULL)
		Tncd_Beacon_Dest = Default_Beacon_Dest;
	if (Tncd_Beacon_Message == NULL)
		Tncd_Beacon_Message = Default_Beacon_Message;
}

void
chk_bbsd_callback(void *obj, void *arg0, int arg1)
{
	char c;

	if(read(bbsd_sock, &c, 1) != 1)
		exit(1);
}

void
build_version_strings(char *me)
{
	char buf[80];
	sprintf(buf, "tncd-%s-M", me);
	strcpy(versionm, daemon_version(buf, Bbs_Call));
	sprintf(buf, "tncd-%s-C", me);
	strcpy(versionc, daemon_version(buf, Bbs_Call));
}

int
main(int argc, char *argv[])
{
	extern int optind;
	struct timeval now, nextexpire, waittime, *pwaittime;
	alCallback cb;
	beacon_task *beacon;
	char *tnc_name;
	slip *master_slip;
	kiss *master_kiss;

	parse_options(argc, argv, ConfigList,
		"TNCD - Terminal Node Controller (KISS) Daemon");

	tnc_name = argv[optind];

	if(bbsd_open(Bbs_Host, Bbsd_Port, tnc_name, "DAEMON") == ERROR)
		error_print_exit(0);

	bbsd_sock = bbsd_socket();
	srandomdev();

	error_clear();

	/* Make it possible to detect if beaconing has been set */
	Tncd_Beacon_Interval = -1;
	/* KISS Mux is disabled by default */
	Tncd_KISS_Mux_Port = 0;

	bbsd_get_configuration(ConfigList);
	bbsd_msg("Startup");

	build_version_strings(tnc_name);
	preload(tnc_name);

	if(dbug_level & dbgTESTHOST)
		test_host(Tncd_Host);
	if(!(dbug_level & dbgFOREGROUND))
		daemon(1, 1);

	if(bbsd_port(Tncd_Monitor_Port))
		error_print_exit(0);

	build_bbscall();

	alEvent_init();

	AL_CALLBACK(&cb, NULL, chk_bbsd_callback);
	int res = alEvent_registerFd(bbsd_sock, ALFD_READ, cb, &bbsd_ev);
	if (res != 0)
		error_print_exit(0);

	if ((Master_ASY = asy_init(Tncd_Device)) == NULL)
		return 1;

	if ((master_slip = slip_init(Tncd_SLIP_Flags)) == NULL)
		return 1;

	if ((master_kiss = kiss_init()) == NULL)
		return 1;

	if(ax_control_init(master_kiss, Tncd_Control_Bind_Addr,
		Tncd_Control_Port) == ERROR)
		return 1;

	if(monitor_init(Tncd_Monitor_Bind_Addr, Tncd_Monitor_Port) == ERROR)
		return 1;

	if(kiss_mux_init(master_kiss, master_slip, Tncd_KISS_Mux_See_Others,
		Tncd_KISS_Mux_Bind_Addr, Tncd_KISS_Mux_Port) == ERROR)
		return 1;

	/* Set upcall chain from TNC into ax25 stack */
	asy_set_recv(Master_ASY, slip_input, master_slip);
	slip_set_recv(master_slip, kiss_nexus_recv, master_kiss);

	/* Set downcall chain from ax25 stack into TNC */
	kiss_set_send(master_kiss, kiss_nexus_send, master_slip);
	slip_set_send(master_slip, asy_send, Master_ASY);

	asy_enable(Master_ASY, 1);
	asy_start(Master_ASY);

	if (Tncd_Beacon_Interval > 0) {
		beacon = beacon_task_new(Tncd_Beacon_Call, Tncd_Beacon_Dest,
			Tncd_Beacon_Message, Tncd_Beacon_Interval, master_kiss);
		if (beacon == NULL)
			return 1;
		if (beacon_task_start(beacon, random() % 30) != 0)
			return 1;
	} else {
		beacon = NULL;
	}

	bbsd_msg("");

	while (alEvent_pending())
		alEvent_poll();

	if (beacon != NULL) {
		beacon_task_stop(beacon);
		beacon_task_free(beacon);
		beacon = NULL;
	}

	asy_stop(Master_ASY);

	kiss_deinit(master_kiss);
	slip_deinit(master_slip);
	asy_deinit(Master_ASY);

	alEvent_shutdown();

	return 0;
}

/* Callback from AX.25 stack for unlock feature */
void
Tncd_TX_Enable(int enable)
{
	asy_enable(Master_ASY, enable);
}

int
Is_Tncd_TX_Enabled(void)
{
	return asy_enabled(Master_ASY);
}
