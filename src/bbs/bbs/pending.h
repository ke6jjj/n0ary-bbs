struct pending_operations {
    struct pending_operations *next;
    int cmd;
    int number;
    char name[256];
	char text[1024];
	struct text_list *tl;
};

extern struct pending_operations
	*queue_pending_operation(int cmd),
    *PendingOp;

extern void
	queue_pending_command(char *cmd);
