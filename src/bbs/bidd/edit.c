#include <stdio.h>
#include <time.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"
#include "bbslib.h"
#include "bid.h"

char *
chk_mid(char *bid)
{
	if(*bid)
		if(hash_get_bid(bid) != NULL)
			return "NO, duplicate\n";
	return "OK\n";
}

char *
chk_bid(char *bid)
{
	struct bid_entry *b;

	if(*bid)
		if((b = hash_get_bid(bid)) != NULL) {
			if(strncmp(b->str, "MID_", 4))
				return "NO, duplicate\n";
		}
	return "OK\n";
}

char *
add_bid(char *bid)
{
	time_t now = Time(NULL);
	struct bid_entry *b;

	if(*bid) {
		if((b = hash_get_bid(bid)) != NULL)
			return "NO, duplicate\n";

		b = hash_create_bid(bid);
		strcpy(b->str, bid);
		b->seen = now;
		bid_image = DIRTY;
	}
	return "OK\n";
}

char *
delete_bid(char *s)
{
	char bid[80];

	strcpy(bid, get_string(&s));
	uppercase(bid);
	if(hash_get_bid(bid) == NULL)
		return "NO, bid not in database\n";
	if(hash_delete_bid(bid) == OK)
		bid_image = DIRTY;

	return "OK\n";
}
