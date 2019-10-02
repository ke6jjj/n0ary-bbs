#include <stdio.h>
#include <string.h>
#include <unistd.h>
#if HAVE_REGCOMP
#include <regex.h>
#endif /* HAVE_REGCOMP */

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "gate.h"

int
disp_guess_on_name(char *s)
{
	int cnt = 0;
	char *p;
	struct gate_entry *g = GateList;
#if HAVE_REGCOMP
	regex_t preg;
	int ret;
#endif /* HAVE_REGCOMP */

#if HAVE_REGCOMP
	if (regcomp(&preg, s, 0) != 0)
		return 0;
#else
	if(re_comp(s))
		return 0;
#endif /* HAVE_REGCOMP */

	while(g) {
		struct gate_entry *a = g;
		while(a) {
			char pattern[256];
			strcpy(pattern, a->addr);
			uppercase(pattern);
			if((p = (char*)index(pattern, '@')) != NULL)
				*p =0;

#if HAVE_REGCOMP
			if (regexec(&preg, pattern, 0, NULL, 0) == 0) {
#else
			if(re_exec(pattern) == 1) {
#endif /* HAVE_REGCOMP */
				if(strlen(output) < 4000) {
					sprintf(output, "%s%s\t%s\n", output, a->call, a->addr);
					cnt++;
				}
			}
			a = a->chain;
		}
		NEXT(g);
	}
#if HAVE_REGCOMP
	regfree(&preg);
#endif /* HAVE_REGCOMP */
	return cnt;
}

int
disp_guess_on_domain(char *s)
{
	int cnt = 0;
	char *p;
	struct gate_entry *g = GateList;
#if HAVE_REGCOMP
	regex_t preg;
	int ret;
#endif /* HAVE_REGCOMP */

#if HAVE_REGCOMP
	if (regcomp(&preg, s, 0) != 0)
		return 0;
#else
	if(re_comp(s))
		return 0;
#endif /* HAVE_REGCOMP */

	if((p = (char*)rindex(s, '.')) != NULL) {
		p--;
		while((int)p > (int)s) {
			if(*p == '.') {
				p++;
#if HAVE_REGCOMP
				regfree(&preg);
				if (regcomp(&preg, p, 0) != 0)
					return 0;
#else
				if(re_comp(p))
					return 0;
#endif /* HAVE_REGCOMP */
				break;
			}
			p--;
		}
	}


	while(g) {
		struct gate_entry *a = g;
		while(a) {
			char pattern[256];
			strcpy(pattern, a->addr);
			uppercase(pattern);
			if((p = (char*)index(pattern, '@')) != NULL)
				p++;
			else
				p = pattern;

#if HAVE_REGCOMP
			if (regexec(&preg, pattern, 0, NULL, 0) == 0) {
#else
			if(re_exec(pattern) == 1) {
#endif /* HAVE_REGCOMP */
				if(strlen(output) < 4000) {
					sprintf(output, "%s%s\t%s\n", output, a->call, a->addr);
					cnt++;
				}
			}
			a = a->chain;
		}
		NEXT(g);
	}
#if HAVE_REGCOMP
	regfree(&preg);
#endif /* HAVE_REGCOMP */
	return cnt;
}

char *
disp_guess(char *s)
{
	char *p, *domain, *user;
	output[0] = 0;

	uppercase(s);
	if((p = (char*)index(s, '\r')) != NULL)
		*p = 0;

	user = s;
	if((domain = (char*)index(s, '@')) != NULL)
		*domain++ = 0;
	
	if(*user) {
		strcat(output, "Possible matches on username:\n");
		disp_guess_on_name(user);
		strcat(output, "\n");
	}

	if(domain) {
		strcat(output, "Possible matches on domain:\n");
		disp_guess_on_domain(domain);
	}

	strcat(output, ".\n");
	return output;
}
