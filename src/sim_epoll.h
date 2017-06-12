#ifndef _DCOM_EPOLL_HXX_
#define _DCOM_EPOLL_HXX_
#include "sim_para.h"

extern int event_add_listen(int epfd,int events,int fd);
extern int event_add(int epfd, int events, link_info_t *plink_info);
extern int event_init(link_info_t *plink_info, int fd);
extern int event_del(int epfd, link_info_t *plink_info);
extern int event_modify(int epfd, int events, link_info_t *plink_info);
extern int event_delete(int epfd, link_info_t *plink_info);


#endif
