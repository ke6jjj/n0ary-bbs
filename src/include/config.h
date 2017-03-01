/* This file is included in every file in the bbs. If a change is made to
 * this file the entire bbs and all the daemons must be recompiled. You
 * do this by typing "make" while in the same directory as this file.
 */

#include "c_cmmn.h"
#include "common.h"

#define BBS_SID     "[ARY-%-H$]"
#define BBS_HOST	"photon"
#define BBSD_PORT	44510

#if 1
#define SOLA_PORT		44400
#define SOLA_DEVICE		"/dev/ttyr1"
#define SOLA_HOST		"solahost"
/* "*" = Be available on network, not just locally */
#define SOLA_BIND_ADDR          "*"

#define DECTALK_PORT	44401
#define DECTALK_DEVICE	"/dev/ttyr9"
#define DECTALK_HOST	"dectalkhost"
/* "*" = Be available on network, not just locally */
#define DECTALK_BIND_ADDR "*"

#define DTMF_PORT		44402
#define DTMF_DEVICE		"/dev/ttyr8"
#define DTMF_HOST		"dtmfhost"
/* "*" = Be available on network, not just locally */
#define DTMF_BIND_ADDR "*"

#define METCON_PORT		44420
#define METCON_DEVICE	"/dev/ttyb"
#define METCON_HOST		"metconhost"
/* "*" = Be available on network, not just locally */
#define METCON_BIND_ADDR "*"

#endif
