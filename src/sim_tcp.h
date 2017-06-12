#ifndef _DCOM_SOCKET_HXX_
#define _DCOM_SOCKET_HXX_

extern int accept_connection(int fd);
extern void *t_recv_handle(void *arg);
extern void *send_handle(void *arg);
extern void *SendData(void *arg);
extern int listen_ports();
extern void *t_iec104_circle(void *arg);
extern int IO_read(const int sockfd, unsigned char *buf, int nbytes);
extern int IO_write(const int sockfd, unsigned char *buf, int nbytes);
extern void *t_send_circle(void *arg);
extern void *t_epoll_send_wait_circle(void *arg);
extern void *t_epoll_recv_wait_circle(void *arg);

#endif

