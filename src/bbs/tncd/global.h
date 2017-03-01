
/* Global definitions used by every source file.
 * Some may be compiler dependent.
 */
#define MK_FP(seg,ofs)	((void far *) \
			   (((unsigned long)(seg) << 16) | (unsigned)(ofs)))
/*wa6ngo* Switch "a" and "b" below as these functions are called with
	  the offset as the first param and the segment as the second*/
#define pokew(a,b,c)	(*((int  far*)MK_FP((b),(a))) = (int)(c))
#define pokeb(a,b,c)	(*((char far*)MK_FP((b),(a))) = (char)(c))
#define peekw(a,b)	(*((int  far*)MK_FP((b),(a))))
#define peekb(a,b)	(*((char far*)MK_FP((b),(a))))
#define movblock(so,ss,do,ds,c)	movedata(ss,so,ds,do,c)

#define outportw outport
#define inportw inport
#define index strchr
#define rindex strrchr

/* Indexes into binmode in files.c; hook for compilers that have special
 * open modes for binary files
 */
#define	READ_BINARY	0
#define	WRITE_BINARY	1
#define APPEND_BINARY   2
extern char *binmode[];

/* not all compilers grok defined() */
#ifdef NODEFINED  /* { */
#define defined(x) (x)
#endif   /* } */

/* These two lines assume that your compiler's longs are 32 bits and
 * shorts are 16 bits. It is already assumed that chars are 8 bits,
 * but it doesn't matter if they're signed or unsigned.
 */
#define	MAXINT16 65535		/* Largest 16-bit integer */


/* General purpose function macros */
#define	min(x,y)	((x)<(y)?(x):(y))	/* Lesser of two args */
#define	max(x,y)	((x)>(y)?(x):(y))	/* Greater of two args */

/* Convert an address to a LONG value for printing */
#define ptr2long(x)	((long) (x))	/* typecast suffices for others */

/* Extract a short from a long */
/* According to my docs, this bug is fixed in MWC 3.0. (from 3.0.6 release
   notes.) -- hyc */
#define hiword(x)	((int16)((x) >> 16))
#define	loword(x)	((int16)(x))

/* Extract a byte from a short */
#define	hibyte(x)	(((x) >> 8) & 0xff)
#define	lobyte(x)	((x) & 0xff)

/* Extract nibbles from a byte */
#define	hinibble(x)	(((x) >> 4) & 0xf)
#define	lonibble(x)	((x) & 0xf)
