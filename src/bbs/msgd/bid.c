#include <stdio.h>
#include <string.h>

#include "bid.h"
#include "tools.h"
#include "c_cmmn.h"

static int global_bid_bootstrap(int initial_bid);
static int global_bid_update(int bid);
static int itob36(char *buf, size_t len, unsigned int i);

static char *Global_Bid_Db_Path;
static int Use_Global_Bids;
static int Last_Global_Bid;

int
initialize_bids(const char *global_bid_db_path, int max_message_id)
{
	FILE *gb;
	int res;
	
	if (global_bid_db_path == NULL) {
		/* System doesn't use a global bid file. */
		Global_Bid_Db_Path = NULL;
		Use_Global_Bids = 0;
		return 0;
	}

	Global_Bid_Db_Path = strdup(global_bid_db_path);
	if (Global_Bid_Db_Path == NULL)
		return -1;

	Use_Global_Bids = 1;

	gb = fopen(Global_Bid_Db_Path, "r");
	if (gb == NULL) {
		/*
		 * We will assume that this system is being brought
		 * into using a global bid file for the first time.
		 */
		return global_bid_bootstrap(max_message_id);
	}

	res = fscanf(gb, "%d", &Last_Global_Bid);
	fclose(gb);

	if (res < 0)
		return -1;

	return 0;
}

int
allocate_bid(int message_number)
{
	int next_bid;

	if (Use_Global_Bids == 0) {
		return message_number;
	}

	next_bid = Last_Global_Bid + 1;

	if (global_bid_update(next_bid) != 0)
		return -1;

	return Last_Global_Bid;
}


static int
global_bid_bootstrap(int last_id)
{
	if (last_id < 0) {
		return -1;
	}

	return global_bid_update(last_id);
}

static int
global_bid_update(int bid)
{
	FILE *gb;
	if ((gb = spool_fopen(Global_Bid_Db_Path)) == NULL) {
		goto SpoolFopenFailed;
	}

	if (fprintf(gb, "%d\n", bid) < 0) {
		goto IdWriteFailed;
	}

	if (spool_fclose(gb) != OK) {
		goto SpoolFcloseFailed;
	}

	Last_Global_Bid = bid;

	return 0;

SpoolFcloseFailed:
	return -1;
IdWriteFailed:
	spool_abort(gb);
SpoolFopenFailed:
	return -1;
}

/*
 * Message numbers 1-99999        : nnnnn_CALL (decimal)
 * Message numbers 100000-60566175: bbbbb.CALL (base36)
 * Message numbers >=60566176     : TODO.
 */
int
format_bid(char *buf, size_t len, unsigned int number, const char *bbs)
{
	size_t actual;

	char b36_buf[6];

	if (number < 100000) {
		actual = snprintf(buf, len, "%u_%s", number, bbs);
		return (actual <= len) ? 0 : -1;
	}

	/* BID needs to use base36 scheme */
	number -= 100000;

	if (itob36(b36_buf, sizeof(b36_buf), number) != 0)
		return -1;

	actual = snprintf(buf, len, "%s.%s", b36_buf, bbs);
	return (actual <= len) ? 0 : -1;
}

static const char b36_alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static int
itob36(char *buf, size_t len, unsigned int i)
{
	size_t p, total_printed;
	unsigned int acc, rem;

	if (len == 0)
		return -1;

	p = len;
	acc = i;

	while (p > 0) {
		rem = acc % 36;
		buf[p-1] = b36_alphabet[rem];
		p = p - 1;
		acc = acc / 36;
		if (acc == 0)
			break;
	}

	if (acc != 0)
		/* Number too big for space provided */
		return -1;

	total_printed = len - p;

	if (p > 0) {
		memmove(&buf[0], &buf[p], total_printed);
	}

	buf[total_printed] = '\0';

	return 0;
}
