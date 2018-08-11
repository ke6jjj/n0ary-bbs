#include <stdio.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "vars.h"


char
	*Bbs_Call,
	*Bbs_MyCall,
	*Bbs_FwdCall,
	*Bbs_Sysop,
	*Bbs_Dir,
	*Bbs_Hloc,
	*Bbs_Domain,
	*Bbs_Header_Comment,
	*Bbs_Motd_File,
	*Bbs_Translate_File,
	*Bbs_NoSub_File,
	*Bbs_History_File,
	*Bbs_History_Path,
	*Bbs_WxLog_File,
	*Bbs_WxData_File,
	*Bbs_WxOutdoor_File,
	*Bbs_WxOutdoorYest_File,
	*Bbs_WxIndoor_File,
	*Bbs_WxIndoorYest_File,
	*Bbs_Sign_Path,
	*Bbs_Callbk_Path,
	*Bbs_Info_Path,
	*Bbs_Distrib_Path,
	*Bbs_Event_Path,
	*Bbs_Event_Dir,
	*Bbs_Vacation_Path,
	*Bbs_FileSys_Path,
	*Bbs_Sysop_Passwd,
	*Bbs_HelpMsg_Index,
	*Bbs_HelpMsg_Data,
	*Bbs_Hloc_Script,
	*Bbs_Personal_File,
	*Bbs_Reject_File,
	*Bbs_Log_Path;

char
	*Msgd_System_File;

int
#if 0
	Smtp_Port = 0,
#endif
	Bbs_Import_Size,
	Bbs_Msg_Loop;

long
	Bbs_Timer_Tnc,
	Bbs_Timer_Phone,
	Bbs_Timer_Console;

struct ConfigurationList ConfigList[] = {
	{ "",						tCOMMENT,	NULL },
	{ "Bbs_Call",				tSTRING,	(int*)&Bbs_Call },
	{ "Bbs_MyCall",				tSTRING,	(int*)&Bbs_MyCall },
	{ "Bbs_FwdCall",			tSTRING,	(int*)&Bbs_FwdCall },
	{ "Bbs_Host",				tSTRING,	(int*)&Bbs_Host },
	{ "Bbs_Sysop",				tSTRING,	(int*)&Bbs_Sysop },
	{ "Bbs_Sysop_Passwd",		tSTRING,	(int*)&Bbs_Sysop_Passwd },
	{ "Bbs_Dir",				tDIRECTORY,	(int*)&Bbs_Dir },
	{ "Bbs_Hloc",				tSTRING,	(int*)&Bbs_Hloc },
	{ "Bbs_Domain",				tSTRING,	(int*)&Bbs_Domain },
	{ "Bbs_Header_Comment",		tSTRING,	(int*)&Bbs_Header_Comment },
	{ "Bbs_Motd_File",			tFILE,		(int*)&Bbs_Motd_File },
	{ "Bbs_Translate_File",		tFILE,		(int*)&Bbs_Translate_File },
	{ "Bbs_NoSub_File",			tFILE,		(int*)&Bbs_NoSub_File },
	{ "Bbs_History_File",		tFILE,		(int*)&Bbs_History_File },
	{ "Bbs_History_Path",		tDIRECTORY,	(int*)&Bbs_History_Path },
	{ "Bbs_WxLog_File",			tFILE,		(int*)&Bbs_WxLog_File },
	{ "Bbs_WxData_File",		tFILE,		(int*)&Bbs_WxData_File },
	{ "Bbs_WxOutdoor_File",		tFILE,		(int*)&Bbs_WxOutdoor_File },
	{ "Bbs_WxOutdoorYest_File",	tFILE,		(int*)&Bbs_WxOutdoorYest_File },
	{ "Bbs_WxIndoor_File",		tFILE,		(int*)&Bbs_WxIndoor_File },
	{ "Bbs_WxIndoorYest_File",	tFILE,		(int*)&Bbs_WxIndoorYest_File },
	{ "Bbs_Sign_Path",			tDIRECTORY,	(int*)&Bbs_Sign_Path },
	{ "Bbs_Callbk_Path",		tDIRECTORY,	(int*)&Bbs_Callbk_Path },
	{ "Bbs_Info_Path",			tDIRECTORY,	(int*)&Bbs_Info_Path },
	{ "Bbs_Distrib_Path",		tDIRECTORY,	(int*)&Bbs_Distrib_Path },
	{ "Bbs_Event_Path",			tDIRECTORY,	(int*)&Bbs_Event_Path },
	{ "Bbs_Event_Dir",			tFILE,		(int*)&Bbs_Event_Dir },
	{ "Bbs_Vacation_Path",		tDIRECTORY,	(int*)&Bbs_Vacation_Path },
	{ "Bbs_FileSys_Path",		tDIRECTORY,	(int*)&Bbs_FileSys_Path },
	{ "Bbs_HelpMsg_Index",		tFILE,		(int*)&Bbs_HelpMsg_Index },
	{ "Bbs_HelpMsg_Data",		tFILE,		(int*)&Bbs_HelpMsg_Data },
	{ "Bbs_Hloc_Script",		tFILE,		(int*)&Bbs_Hloc_Script },
	{ "Bbs_Personal_File",		tFILE,		(int*)&Bbs_Personal_File },
	{ "Bbs_Reject_File",		tFILE,		(int*)&Bbs_Reject_File },
	{ "Bbs_Log_Path",			tDIRECTORY,	(int*)&Bbs_Log_Path },
	{ "Msgd_System_File",		tFILE,		(int*)&Msgd_System_File },
	{ "Bbs_Import_Size",		tINT,		(int*)&Bbs_Import_Size },
	{ "Bbs_Msg_Loop",			tINT,		(int*)&Bbs_Msg_Loop },
	{ "Bbsd_Port",				tINT,		(int*)&Bbsd_Port },
#if 0
	{ "Smtp_Port",				tINT,		(int*)&Smtp_Port },
#endif
	{ "Bbs_Timer_Tnc",			tTIME,		(int*)&Bbs_Timer_Tnc },
	{ "Bbs_Timer_Phone",		tTIME,		(int*)&Bbs_Timer_Phone },
	{ "Bbs_Timer_Console",		tTIME,		(int*)&Bbs_Timer_Console },

	{ NULL, 0, NULL}};

