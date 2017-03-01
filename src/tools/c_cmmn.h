#include <stdio.h>
#include <string.h>

#define OK	0
#define TRUE	1
#define ERROR	(-1)
#define FALSE	0

#define malloc_struct(s)	(struct s *)mem_calloc(1, sizeof(struct s))
#define malloc_multi_struct(n,s)	(struct s *)mem_calloc(n, sizeof(struct s))

#define NEXT(x)			x = x->next
#define PREV(x)			x = x->last

#define tMin			60
#define tHour			(60*tMin)
#define tDay			(24*tHour)
#define tWeek			(7*tDay)
#define tMonth			(30*tDay)
#define tYear			(365*tDay)

#define ToUpper(s)	if(islower(s)) s = toupper(s)
#define ToLower(s)	if(isupper(s)) s = tolower(s)

#define NextChar(s)             while(isspace(*(s)) && *(s)) { (s)++; }
#define NextSpace(s)            while(!isspace(*(s)) && *(s)) { (s)++; }
#define LastChar(s, b)          while(isspace(*(s)) && (s)>(b)) { (s)--; }
#define LastSpace(s, b)         while(!isspace(*(s)) && (s)>(b)) { (s)--; }

