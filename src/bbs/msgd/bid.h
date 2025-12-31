/*
 * Format a BID generated at this BBS in a manner that is unique across
 * at least 60 million messages, perhaps even more.
 */
int format_bid(char *buf, size_t len, unsigned int number, const char *bbs);

/*
 * Allocate a new unique bulletin id for a bulletin being issued from this
 * system, possibly reverting to the message number itself, if no system
 * for handing out guaranteed unique bids is in place.
 */
int allocate_bid(int message_number);

int initialize_bids(const char *bid_db_path, int fallback_id);
