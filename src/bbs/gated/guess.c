#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

	if(re_comp(s))
		return 0;

	while(g) {
		struct gate_entry *a = g;
		while(a) {
			char pattern[256];
			strcpy(pattern, a->addr);
			uppercase(pattern);
			if((p = (char*)index(pattern, '@')) != NULL)
				*p =0;

			if(re_exec(pattern) == 1) {
				if(strlen(output) < 4000) {
					sprintf(output, "%s%s\t%s\n", output, a->call, a->addr);
					cnt++;
				}
			}
			a = a->chain;
		}
		NEXT(g);
	}
	return cnt;
}

int
disp_guess_on_domain(char *s)
{
	int cnt = 0;
	char *p;
	struct gate_entry *g = GateList;

	if(re_comp(s))
		return 0;

	if((p = (char*)rindex(s, '.')) != NULL) {
		p--;
		while((int)p > (int)s) {
			if(*p == '.') {
				p++;
				if(re_comp(p))
					return 0;
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

			if(re_exec(pattern) == 1) {
				if(strlen(output) < 4000) {
					sprintf(output, "%s%s\t%s\n", output, a->call, a->addr);
					cnt++;
				}
			}
			a = a->chain;
		}
		NEXT(g);
	}
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
