int slip_start(int dev);
int slip_stop(int dev);
int slip_raw(int dev, struct mbuf *data);
int slip_init(int dev);

#define SLIP_FLAGS_ESCAPE_ASCII_C 0x1

extern int Tncd_SLIP_Flags;

int slip_set_flags(int dev, int flags);
