/*
 * Format a BID generated at this BBS in a manner that is unique across
 * at least 60 million messages, perhaps even more.
 */
int format_bid(char *buf, size_t len, unsigned int number, const char *bbs);
