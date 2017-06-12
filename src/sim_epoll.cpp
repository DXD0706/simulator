// add/mod an event to epoll
#include "sim_para.h"

int event_add_listen(int epfd, int events, int fd)
{
    struct epoll_event epv = {0, {0}};

    epv.data.fd = fd;
    epv.events = events;

    if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &epv) < 0) {
        perror("epoll_ctl");
        printf("event_add_listen failed[fd=%d]\n",fd);
        return -1;
    }
    return 0;
}


int event_add(int epfd, int events, link_info_t *plink_info)
{
    struct epoll_event epv = {0, {0}};
    int op;
    epv.data.ptr = plink_info;
    epv.events = plink_info->events = events;
    if(plink_info->is_inpoll == 1 ) {
        op = EPOLL_CTL_MOD;
    } else {
        op = EPOLL_CTL_ADD;
        plink_info->is_inpoll = 1;
    }
    if(epoll_ctl(epfd, op, plink_info->fd, &epv) < 0) {
        perror("epoll_ctl");
        printf("Event Add failed[fd=%d]\n", plink_info->fd);
        return -1;
    }
    return 0;
}
// set event

int event_init(link_info_t *plink_info, int fd)
{
    plink_info->fd = fd;
    plink_info->events = 0;
    return 0;
}


int event_del(int epfd, link_info_t *plink_info)
{
    struct epoll_event epv = {0, {0}};
    if(plink_info->is_inpoll != 1) return -1;  // todo
    epv.data.ptr = plink_info;
    plink_info->is_inpoll = 0;
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, plink_info->fd, &epv);
    if (ret != 0) {
        perror("epoll_ctl");
        printf("event_del failed\n");
        return -1;
    }
    return 0;
}

int event_modify(int epfd, int events, link_info_t *plink_info)
{
    struct epoll_event epv = {0, {0}};
    int op;
    epv.data.ptr = plink_info;
    epv.events = plink_info->events = events;
    if(plink_info->is_in_send_pool == 1 ) {
        op = EPOLL_CTL_MOD;
    } else {
        op = EPOLL_CTL_ADD;
        plink_info->is_in_send_pool = 1;
    }
    int ret = epoll_ctl(epfd, op, plink_info->fd, &epv);
    if( ret< 0)
        printf("event_modify failed[fd=%d]\n", plink_info->fd);
    return ret;
}

int event_delete(int epfd, link_info_t *plink_info)
{
    struct epoll_event epv = {0, {0}};
    if(plink_info->is_in_send_pool != 1) return 0;  // todo
    epv.data.ptr = plink_info;
    plink_info->is_in_send_pool = 0;
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, plink_info->fd, &epv);
    if (ret != 0) {
        perror("epoll_ctl");
        printf("event_delete failed\n");
    }
    return ret;
}


