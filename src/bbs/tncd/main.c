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

#include "ax_mbx.h"

char
    versionm[80],
    versionc[80],
    *Bbs_Call,
	*Bbs_My_Call,
	*Bbs_Fwd_Call,
	*Bbs_Dir,
	*Bin_Dir,
	*Default_Beacon_Call,
	*Default_Beacon_Dest,
	*Default_Beacon_Message,
	*Tncd_Beacon_Call,
	*Tncd_Beacon_Dest,
	*Tncd_Beacon_Message,
	*Tncd_Control_Bind_Addr = NULL,
	*Tncd_Monitor_Bind_Addr = NULL,
	*Tncd_Device;

int
	Tncd_Control_Port,
	Tncd_Monitor_Port,
	Tncd_Maxframe,
	Tncd_N2,
	Tncd_Paclen,
	Tncd_Pthresh,
	Tncd_Beacon_Interval,
	Default_Beacon_Interval;

int
	bbsd_sock,
	Tncd_T1init,
	Tncd_T2init,
	Tncd_T3init;

int background = 0;
int shutdown = FALSE;

alEventHandle bbsd_ev;

struct ConfigurationList ConfigList[] = {
	{ "",						tCOMMENT,	NULL },
	{ "BBS_MYCALL",				tSTRING,	(int*)&Bbs_My_Call },
	{ "BBS_FWDCALL",			tSTRING,	(int*)&Bbs_Fwd_Call },
	{ "BBS_HOST",				tSTRING,	(int*)&Bbs_Host },
	{ "BBS_DIR",				tSTRING,	(int*)&Bbs_Dir },
	{ "BBS_CALL",				tSTRING,	(int*)&Bbs_Call },
	{ "BIN_DIR",				tDIRECTORY,		(int*)&Bin_Dir },
	{ "BBSD_PORT",				tINT,		(int*)&Bbsd_Port },
	{ "AX25_BEACON_INTERVAL",		tSTRING,	(int*)&Default_Beacon_Interval },
	{ "AX25_BEACON_MESSAGE",		tSTRING,	(int*)&Default_Beacon_Message },
	{ "AX25_BEACON_CALL",			tSTRING,	(int*)&Default_Beacon_Call },
	{ "AX25_BEACON_DEST",			tSTRING,	(int*)&Default_Beacon_Dest },
	{ NULL, 0, NULL}};

struct ConfigurationList DynamicConfigList[] = {
	{ "",						tCOMMENT,	NULL },
	{ " Prefix each entry name below with \"TNCx_\" (where x)",tCOMMENT,NULL },
	{ " is a valid TNC port, ie. TNC2_DEVICE.",	tCOMMENT,	NULL },
	{ "",						tCOMMENT,	NULL },
	{ "CONTROL_PORT",		tINT,		(int*)&Tncd_Control_Port },
	{ "CONTROL_BIND_ADDR",		tSTRING,	(int*)&Tncd_Control_Bind_Addr },
	{ "MONITOR_PORT",		tINT,		(int*)&Tncd_Monitor_Port },
	{ "MONITOR_BIND_ADDR",		tSTRING,	(int*)&Tncd_Monitor_Bind_Addr },
	{ "DEVICE",			tSTRING,	(int*)&Tncd_Device },
	{ "",				tCOMMENT,	NULL },
	{ "  AX25 parameters",		tCOMMENT,	NULL },
	{ "",				tCOMMENT,	NULL },
	{ "T1INIT",			tTIME,		(int*)&Tncd_T1init },
	{ "T2INIT",			tTIME,		(int*)&Tncd_T2init },
	{ "T3INIT",			tTIME,		(int*)&Tncd_T3init },
	{ "MAXFRAME",			tINT,		(int*)&Tncd_Maxframe },
	{ "N2",				tINT,		(int*)&Tncd_N2 },
	{ "PACLEN",			tINT,		(int*)&Tncd_Paclen },
	{ "PTHRESH",			tINT,		(int*)&Tncd_Pthresh },
	{ "FLAGS",			tINT,		(int*)&Tncd_SLIP_Flags },
	{ "BEACON_INTERVAL",		tTIME,		(int*)&Tncd_Beacon_Interval },
	{ "BEACON_CALL",		tSTRING,	(int*)&Tncd_Beacon_Call },
	{ "BEACON_DEST",		tSTRING,	(int*)&Tncd_Beacon_Dest },
	{ "BEACON_MESSAGE",		tSTRING,	(int*)&Tncd_Beacon_Message },
	{ NULL, 0, NULL}};

static void
preload(char *name)
{
	int i = 0;
	struct ax25_params *tax = tnc_ax25(name);
	struct ConfigurationList dynconfig[2], *cfg;
	char varname[256];

	Tncd_Control_Bind_Addr = tnc_control_bind_addr(name);
	Tncd_Control_Port = tnc_port(name);
	Tncd_Monitor_Bind_Addr = tnc_monitor_bind_addr(name);
	Tncd_Monitor_Port = tnc_monitor_port(name);
	Tncd_Device = tnc_device(name);
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

	if(ax_control_init(Tncd_Control_Bind_Addr, Tncd_Control_Port) == ERROR)
		return 1;

	if(monitor_init(Tncd_Monitor_Bind_Addr, Tncd_Monitor_Port) == ERROR)
		return 1;

	tnc[0].inuse = FALSE;

	asy_init(0, Tncd_Device);
	slip_init(0);
	slip_start(0);

	if (Tncd_Beacon_Interval > 0) {
		beacon = beacon_task_new(Tncd_Beacon_Call, Tncd_Beacon_Dest,
			Tncd_Beacon_Message, Tncd_Beacon_Interval, 0);
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

	slip_stop(0);

	alEvent_shutdown();

	return 0;
}
