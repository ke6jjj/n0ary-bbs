#if 0
#define SizeofCALLBK	sizeof(struct callbook_entry)
#else
#define SizeofCALLBK	125
#endif

#define SizeofCBINDEX sizeof(struct callbook_index)

extern int
	callbk(void),
	lookup_call_callbk(char *call, struct callbook_entry *cb);

extern void
	gen_callbk_body(struct text_line **tl),
	callbk_server(int num, char *mode);
