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

#include "ax_mbx.h"

char
    versionm[80],
    versionc[80],
    *Bbs_Call,
	*Bbs_My_Call,
	*Bbs_Fwd_Call,
	*Bbs_Dir,
	*Bin_Dir,
	*Tncd_Control_Bind_Addr = NULL,
	*Tncd_Monitor_Bind_Addr = NULL,
	*Tncd_Device;

int
	Tncd_Control_Port,
	Tncd_Monitor_Port,
	Tncd_Maxframe,
	Tncd_N2,
	Tncd_Paclen,
	Tncd_Pthresh;

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
#if 0
	{ "",						tCOMMENT,	NULL },
	{ " Replace the TNCxD below with a entry for each of the",tCOMMENT,NULL },
	{ " valid TNC ports, ie. TNC2D_DEVICE.",	tCOMMENT,	NULL },
	{ "",						tCOMMENT,	NULL },
	{ "TNCxD_CONTROL_BIND_ADDR",	tSTRING,	(int*)&Tncd_Control_Bind_Addr },
	{ "TNCxD_CONTROL_PORT",		tINT,		(int*)&Tncd_Control_Port },
	{ "TNCxD_MONITOR_BIND_ADDR",	tSTRING,	(int*)&Tncd_Monitor_Bind_Addr },
	{ "TNCxD_MONITOR_PORT",		tINT,		(int*)&Tncd_Monitor_Port },
	{ "TNCxD_DEVICE",			tSTRING,	(int*)&Tncd_Device },
	{ "",						tCOMMENT,	NULL },
	{ "  AX25 parameters",		tCOMMENT,	NULL },
	{ "",						tCOMMENT,	NULL },
	{ "TNCxD_T1INIT",			tTIME,		(int*)&Tncd_T1init },
	{ "TNCxD_T2INIT",			tTIME,		(int*)&Tncd_T2init },
	{ "TNCxD_T3INIT",			tTIME,		(int*)&Tncd_T3init },
	{ "TNCxD_MAXFRAME",			tINT,		(int*)&Tncd_Maxframe },
	{ "TNCxD_N2",				tINT,		(int*)&Tncd_N2 },
	{ "TNCxD_PACLEN",			tINT,		(int*)&Tncd_Paclen },
	{ "TNCxD_PTHRESH",			tINT,		(int*)&Tncd_Pthresh },
	{ "TNCxD_FLAGS",			tINT,		(int*)&Tncd_SLIP_Flags },
#endif
	{ NULL, 0, NULL}};

static void
preload(char *name)
{
	int i = 0;
	struct Tnc_ax25 *tax = tnc_ax25(name);

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
	Tncd_Paclen = Tncd_Pthresh = tax->paclen;
	Tncd_SLIP_Flags = tax->flags;

	for(i=0; ConfigList[i].token != NULL; i++) {
		if(!strncmp(ConfigList[i].token, "TNCx", 4))
			ConfigList[i].token[3] = name[3];
	}		
}

void
chk_bbsd_callback(void *obj, void *arg0, int arg1)
{
	char c;

	if(read(bbsd_sock, &c, 1) == ERROR)
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


	parse_options(argc, argv, ConfigList,
		"TNCD - Terminal Node Controller (KISS) Daemon");

	if(bbsd_open(Bbs_Host, Bbsd_Port, argv[optind], "DAEMON") == ERROR)
		error_print_exit(0);

	bbsd_sock = bbsd_socket();

	error_clear();
	bbsd_get_configuration(ConfigList);
	bbsd_msg("Startup");

	build_version_strings(argv[optind]);
	preload(argv[optind]);

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
	slip_start(0);

	bbsd_msg("");

	while (alEvent_pending())
		alEvent_poll();

	slip_stop(0);

	alEvent_shutdown();

	return 0;
}
