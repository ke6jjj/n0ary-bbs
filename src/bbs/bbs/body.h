
#define mINCLUDE		1
#define mABORT			2
#define mTERM			3
#define mTERMwrite		4
#define mCC				5
#define mROUTING		6
#define mREADFILE		7

#define mLISTNUMBERS		8
#define mEDITLINE		9
#define mKILLLINE		10
#define mADDLINE		11
#define mSIGNATURE		12
#define mNOSIGNATURE	13
#define mHELP			14

#define MSG_BODY		1
#define FILE_BODY		2
#define DISTRIB_BODY	3
#define EVENT_BODY		4

#include <sys/types.h>

extern int
	include_file(char *buf),
#ifdef NOMSGD
	get_body(FILE *fp, int type, char *orig_bbs, int *orig_num, time_t *orig_date),
#else
	get_body(struct text_line **tl, int type, char *orig_bbs, int *orig_num, time_t *orig_date),
#endif
	msg_line_type(char *str, int type);

extern void
	link_line(char *str);
