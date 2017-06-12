#ifndef _SIM_COMMOND_H_
#define _SIM_COMMOND_H_

extern int recv_commond_frame(int fd, unsigned char *buf, int len);

extern void *t_listen_commond(void *arg);

#endif
