extern int
	asy_set_read_cb(int dev, void (*cb)(int dev, void *), void *arg),
	asy_recv(int dev, char *buf, int cnt),
	asy_output(int dev, char *buf, int cnt),
	asy_init(int dev, char *ttydev);

extern int Tncd_TX_Enabled;
