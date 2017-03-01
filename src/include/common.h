#define FOREGROUND	(dbug_level & dbgFOREGROUND)
#define VERBOSE		(dbug_level & dbgVERBOSE && FOREGROUND)

#define Prog_BBS		1
#define Prog_IMPORT		2
#define Prog_CALLBK     3
#define Prog_WP         4
#define Prog_REMOTE		5
#define Prog_FWD		6

#define PORT0		0x01
#define PORT1		0x02
#define PORT2		0x04
#define PORT3		0x08
#define PORT4		0x10
#define PORT5		0x20
#define PORT6		0x40
#define PORT7		0x80
#define PORT8		0x100
#define PORT9		0x200
#define PORT10		0x400
#define PORT11		0x800
#define PORT12		0x1000
#define PORT13		0x2000
#define PORT14		0x4000
#define PORT15		0x8000
#define PORT16		0x10000
#define PORT17		0x20000
#define PORT18		0x40000
#define PORT19		0x80000
#define PORT20		0x100000
#define PORT21		0x200000
#define PORT22		0x400000
#define PORT23		0x800000
#define PORT24		0x1000000
#define PORT25		0x2000000
#define PORT26		0x4000000
#define PORT27		0x8000000
#define PORT28		0x10000000
#define PORT29		0x20000000
#define PORT30		0x40000000
#define PORT31		0x80000000

#define tTNC		1
#define tPHONE		2
#define tCONSOLE	3
#define tSMTP		4
#define tTCP		5

extern int
	batch_mode,
	PendingCmd,
	Program;

#define CapAsIs		0
#define CapFirst	1
#define AllLowerCase	2
#define AllUpperCase	3

#define isOK(p)		((*p == 'O') && (*(p+1) == 'K'))
#define isERROR(p)	(!isOK(p))

#define YES		0
#define NO		1
#define QUIT		2

#define	LenCALL		7
#define	LenSUBJECT	60
#define	LenFNAME	20
#define	LenLNAME	20
#define	LenQTH		30
#define	LenZIP		6
#define	LenHOME		7
#define	LenPHONE	15
#define	LenEQUIP	40
#define	LenCLUB		10
#define	LenMACRO	80
#define	LenPASSWD	9
#define	LenEMAIL	80
#define	LenFREQ		80
#define LenHLOC		40
#define LenBID		20
#define LenINCLUDE	150

#define LenMAX		LenINCLUDE

#define PRINTF		print_socket
#define PRINT 		putchar_socket
#define GETS		get_socket

extern short bbscallsum;

extern int
	override_help_level,
	disable_help,
	no_prompt,
	preempt_init_more,
	more_cnt,
	maintenance_mode,
	bbs_fwd_mask;

extern char *
	get_call(char **str);

#if 0
struct call_and_checksum {
	char	str[LenCALL];
	short	sum;
};

struct PortDefinition {
	struct PortDefinition *next;
	int secure, type;
	char *name;
	char alias[9];
	int show;
};


struct Tnc_ax25 {
		/* AX25 parameters
		 * t1 = This is how long to wait after a frame is sent before
		 *      sending a poll request for error recovery.
		 * t2 = The amount of time to wait after a frame is received for
		 *		another frame. If the timer expires before another frame
		 *		is received an ACK is sent.
		 * t3 = How often to ping the remote site if no other frame
		 *		has been received.
		 * maxframe = maximum number of frames to send in a row without
		 *				requiring an acknowlegement.
		 * paclen = maximum packet size in bytes. Typical setting would
		 *			be 110, should never go beyond 256.
		 * n2 = number of retries to allow.
		 */

	int t1, t2, t3;
	int maxframe, paclen;
	int n2;
};

struct TncDefinition {
	struct TncDefinition *next;
	char *name;
	int port;
	int monitor;
	char *device;
	char *host;
	struct Tnc_ax25 ax25;
};
	
struct PhoneDefinition {
	struct PhoneDefinition *next;
	char *name;
	char *device;
	char *init_str;
};


#endif
