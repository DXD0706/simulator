#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "sim_para.h"
#include "sim_epoll.h"
#include "sim_thread.h"
#include "sim_iec104.h"
#include "sim_tcp.h"
#include "sim_log.h"


typedef struct listen_t {
    unsigned short port;
    struct listen_t *pNext;
} t_listen;


int IO_write(const int sockfd , unsigned char  *buf, int len)
{

    int nwritten = 0;
    int nleft = 0;
    u_char *ptr;
    ptr = (u_char *)buf;
    nleft = len;

    while(nleft > 0) {
        nwritten = send(sockfd, ptr, nleft, 0);
        if(nwritten < 0) {
            if(errno == EINTR) {
                continue;
            } else {
                return -1;
            }
        }
        return nwritten;
    }
    return len;
}

int IO_read(const int sockfd, unsigned char *buf, int nbytes)
{
    int nleft, nread;
    nleft = nbytes;
    while (nleft > 0) {
        nread = recv(sockfd, buf, nleft, 0);
        if (nread < 0) {
            if(errno == EAGAIN) {
                int n = nbytes - nleft;
                assert(n >= 0);
                return n;
            } else {
                return -1;
            }
        }
        if (nread == 0) {
            return -2;
        }

        nleft -= nread;
        buf += nread;
    }
    return (nbytes - nleft);
}



int link_shut(link_info_t *plink_info)
{
    if (plink_info->tcp_status == SOCKET_CLOSED)
        return 0;

    plink_info->tcp_status = SOCKET_CLOSED;
    plink_info->proto_status = PROTO_STATUS_UNINIT;

    int i = plink_info->linkno % MAXRECVTHREAD;

    event_del(g_epfd_recv[i], plink_info);
    event_delete(g_epfd_send, plink_info);
    pthread_mutex_lock(&(plink_info->fdlock));
    close (plink_info->fd);
    plink_info->fd = -1;
    plink_info->len_r = 0;
    plink_info->u_test_send_flag = 0;
    plink_info->u_ack_timeout_count = 0;
    pthread_mutex_unlock(&(plink_info->fdlock));
    fprintf(fp_net, "linkno:%05d shut down\n", plink_info->linkno);
    fflush(fp_net);
    return 0;
}


int link_info_dispatch(link_info_t *plink_info)
{
    int i = plink_info->linkno % MAXRECVTHREAD;

    plink_info->send_no = 0;
    plink_info->recv_no = 0;
    plink_info->r_idlesse_count = 0;
    plink_info->u_test_send_flag = 0;
    plink_info->u_ack_timeout_count = 0;
    plink_info->tcp_status = SOCKET_CONNECTED;
    plink_info->len_r = 0;
    plink_info->is_recving = 0x00;
    plink_info->events = 0;
    event_add(g_epfd_recv[i], EPOLLIN | EPOLLET | EPOLLHUP, plink_info);
    return 0;
}

link_info_t *alloc_link()
{
    for_each_link(g_linkMap) {
        link_info_t *plink_info = it->second;
        if (plink_info->tcp_status == SOCKET_CLOSED) {
            plink_info->tcp_status = SOCKET_CONNECTED;
            return plink_info;
        }
    }
    return NULL;
}

int listen_ports()
{
    int listenFd;
    sockaddr_in sin;
    int Reuseaddr = 1;

    for(set<unsigned short>::iterator it = g_portSet.begin(); it != g_portSet.end(); it++) {

        unsigned short port = *it;
        if((listenFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket error:");
            exit(1);
        }
#ifdef __linux__
        if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &Reuseaddr, sizeof(Reuseaddr)) < 0)
#else
        if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEPORT, &Reuseaddr, sizeof(Reuseaddr)) < 0)
#endif
        {
            perror("setsockopt SO_REUSEADDR");
            exit(1);
        }

        if(fcntl(listenFd, F_SETFL, O_NONBLOCK) == -1) { // set non-blocking
            perror("fcntl error");
            exit(1);
        }

        bzero(&sin, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(port);

        if(bind(listenFd, (const sockaddr *)&sin, sizeof(sin)) == -1) {
            char str[32];
            snprintf(str, sizeof(str), "port %d bind error", port);
            perror(str);
            close(listenFd);
            continue;
        }
        if(listen(listenFd, 511) == -1) {
            perror("listen error");
            close(listenFd);
            continue;
        }

        event_add_listen(g_epfd_listen, EPOLLIN | EPOLLET, listenFd);
    }
    printf("listen ports\t\t[ ok ]\n");
    return 0;
}


int accept_connection(int fd)
{
    struct sockaddr_in sin;
    char str_addr[INET_ADDRSTRLEN];
    socklen_t len = sizeof(struct sockaddr_in);
    int nfd;
    link_info_t *plink_info = NULL;
    time_t time_now = time(NULL);

    while(sig_flag) {

#ifdef __linux__
        if((nfd = accept(fd, (struct sockaddr *)&sin, (socklen_t *)&len)) == -1)
#else
        if((nfd = accept(fd, (struct sockaddr *)&sin, (int *)&len)) == -1)
#endif
        {
            if(errno == EINTR)
                continue;
            else {
                return -1;
            }
        }

        if(inet_ntop(AF_INET, &sin.sin_addr, str_addr, sizeof(str_addr)) == NULL) {
            perror("inet_ntop");
            close(nfd);
            continue;
        }

        if(fcntl(nfd, F_SETFL, O_NONBLOCK) < 0) {
            close(nfd);
            continue;
        }
        plink_info = alloc_link();
        if (plink_info == NULL) {
            fprintf(fp_net, "refused one connection from %s,for no link is free\tTIME:%s", str_addr, ctime(&time_now));
            fflush(fp_net);
            close(nfd);
            close(fd);
            continue;
        }
        plink_info->fd = nfd;
        gettimeofday(&(plink_info->conn_time), NULL);

        link_info_dispatch(plink_info);
        fprintf(fp_net, "linkno:%04d connect open at [%s] \tTIME:%s", plink_info->linkno, str_addr, ctime(&time_now));
        fflush(fp_net);
    }
    return 0;

}

int recv_data(link_info_t *plink_info)
{
    ssize_t n = 0;
    int length = 0;
    int reterr = 0;
    int left_len = 0;
    int err_no = 0;

    if (plink_info == NULL) {
        return -1;
    }

    int fd = plink_info->fd;
    //FILE *fp_msg = plink_info->fp_msg;
    unsigned char  *buff_r  = plink_info->buff_r;
    unsigned short *len_r   = &plink_info->len_r;

    while (sig_flag) {
        reterr = 0;
        length = 2;

        if(plink_info->len_r == 0) {
            length = 2;
            n = IO_read(fd, buff_r, length);
            err_no = errno;
            if (n < 0) {
                reterr = -1;
                break;
            } else if (n == 0) {
                reterr = 0;
                break;
            }

            if (buff_r[0] != 0x68 && buff_r[0] != 0x16) {
                reterr = -2;
                log_write_hex(plink_info, DIR_RECV, buff_r, 1);
                break;
            }

            if(n == 1) {
                *len_r = 1;
                reterr = 0;
                break;
            }
            length = buff_r[1];

            if (length < 4 || length > 253) {
                reterr = -3;
                break;
            }

            *len_r = 2;
        }

        if(*len_r == 1) {
            length = 1;
            n = IO_read(fd, &buff_r[1], length);
            err_no = errno;
            if (n < 0) {
                reterr = -5;
                break;
            } else if (n == 0) {
                reterr = 0;
                break;
            }

            length = buff_r[1];
            if (length < 4 || length > 253) {
                reterr = -6;
                log_write_hex(plink_info, DIR_RECV, plink_info->buff_r, 2);
                break;
            }
            *len_r = 2;
        }

        left_len = buff_r[1] + 2 - *len_r;
        if (buff_r[0] == 0x16) {
            left_len += 4;
        }

        n = IO_read(fd, &buff_r[*len_r], left_len);
        err_no = errno;
        if (n < 0) {
            reterr = -8;
            log_write_hex(plink_info, DIR_RECV, buff_r, *len_r);
            break;
        } else if(n < left_len) {
            *len_r += n;
            reterr = 0;
            break;
        }

        *len_r += n;

        log_write_hex(plink_info, DIR_RECV, buff_r, plink_info->len_r);
        analysis_iec104_frame(plink_info, buff_r, plink_info->len_r);
        plink_info->len_r = 0;
        plink_info->u_test_send_flag = 0;
        plink_info->u_ack_timeout_count = 0;
        plink_info->r_idlesse_count = 0;

    }

    return 0;
}


/**
* return value -1 :error 0:block or no data ;>0 send ok
**/
int send_data(link_info_t* p_link)
{
    if (p_link == NULL) {
        errno = EINVAL;
        return -1;
    }

    if(p_link->fd == -1) {
        return -1;
    }

    if (p_link->send_list == NULL) {
        if (p_link->is_in_send_pool != 0) {
            event_delete(g_epfd_send, p_link);
            if (p_link->linkno == 1) {
                printf("delete from send poll\n");
            }
        }
        return 0;
    }
    mem_block_t *mem_block = p_link->send_list;

    int len = IO_write(p_link->fd, &(mem_block->data[mem_block->offset]), mem_block->datalen);
    int errno_m = errno;

    if(len >= 0) {
        log_write_hex(p_link, DIR_SEND, &(mem_block->data[mem_block->offset]), len);

        if (len < mem_block->datalen) {
            mem_block->offset += len;
            mem_block->datalen = mem_block->datalen - len;
            if (p_link->is_in_send_pool == 0) {
                event_modify( g_epfd_send, EPOLLOUT, p_link);
                if (p_link->linkno == 1) {
                    printf("add into send poll\n");
                }
            }
            return 0;
        } else {
            pthread_mutex_lock(&p_link->send_list_mutex);
            p_link->send_list = mem_block->next;
            pthread_mutex_unlock(&p_link->send_list_mutex);
            mem_pool_free(g_send_pool, mem_block);

            return len;
        }
    } else if (len == -1 && errno_m == EAGAIN) {
        if (p_link->is_in_send_pool == 0) {

            event_modify( g_epfd_send, EPOLLOUT, p_link);
            if (p_link->linkno == 1) {
                printf("add into send poll\n");
            }
        }
        return 0;
    } else {
        time_t timenow = time(NULL);
        fprintf(fp_net, "linkno:%04d connection closed while writting %s \tTIME:%s", p_link->linkno, strerror(errno_m), ctime(&timenow));
        fflush(fp_net);
        link_shut(p_link);
    }
    return -1;

}

void *t_send_circle(void *arg)
{
    pthread_detach(pthread_self());

    link_info_t *p_link;
    int send_count = 0;
    int send_thread_id = (long)arg;
    struct timezone tz;
    struct timeval tv1;
    struct timeval tv2;
    struct timeval tva;
    struct timeval tvb;
    log_write_pid();

    while(1) {
        send_count = 0;
        gettimeofday(&tva, &tz);
        long msa = (tva.tv_sec - tvb.tv_sec) * 1000 + (tva.tv_usec - tvb.tv_usec) / 1000;
        if (msa > 1) {
            //printf("sleep used %d ms\n",msa);
        }
        for_each_link(g_linkMap) {
            int linkno = it->first;
            p_link = it->second;
            assert(linkno == p_link->linkno);
            if (p_link->linkno % MAXSENDTHREAD != send_thread_id) continue;
            if (p_link->fd < 0)                         continue;
            if (p_link->tcp_status != SOCKET_CONNECTED) continue;			
            if (p_link->is_in_send_pool != 0)           continue;
            if (p_link->send_list == NULL)              continue;
            while (1) {
                gettimeofday(&tv1, &tz);
                int len = send_data(p_link);
                gettimeofday(&tv2, &tz);
                if (len > 0) {
                    send_count++;
                }
                long ms = (tv2.tv_sec - tv1.tv_sec) * 1000 + (tv2.tv_usec - tv1.tv_usec) / 1000;
                if (ms > 0) {
                    //printf("send_data used %d ms\n",ms);
                }

                break;
            }
        }
        gettimeofday(&tvb, &tz);
        long ms = (tvb.tv_sec - tva.tv_sec) * 1000 + (tvb.tv_usec - tva.tv_usec) / 1000;
        if (ms > 10) {
            //printf("send circle used %d ms\n",ms);
        }
        if (send_count == 0)
            usleep(100);
    }
    return NULL;
}


void *t_epoll_recv_wait_circle(void *arg)
{
    pthread_detach(pthread_self());

    struct epoll_event events[MAX_WAIT_EVT];
    int epfd = *(int*)arg;
    log_write_pid();

    while(sig_flag) {
        int n = epoll_wait(epfd, events, MAX_WAIT_EVT, 1000);
        if(n < 0) {
            if (errno == EINTR) continue;
            perror("g_epfd_send epoll_wait");
            printf("g_epfd_send epoll_wait error, exit\n");
            break;
        }

        for(int i = 0; i < n; i++) {
            link_info_t *plink_info = (link_info_t *)events[i].data.ptr;
            if(plink_info->fd < 0)
                continue;
            if((events[i].events & EPOLLIN) && (plink_info->events & EPOLLIN)) { // read event
                recv_data(plink_info);
            }
        }
    }
    return NULL;
}


void *t_epoll_send_wait_circle(void *arg)
{
    pthread_detach(pthread_self());

    link_info_t *p_link;
    int send_count = 0;

    log_write_pid();

    struct epoll_event events[MAX_WAIT_EVT];
    while(sig_flag) {
        int fds = epoll_wait(g_epfd_send, events, MAX_WAIT_EVT, 1000);
        if(fds < 0) {
            if (errno == EINTR) continue;
            perror("g_epfd_send epoll_wait");
            printf("g_epfd_send epoll_wait error, exit\n");
            break;
        }

        for(int i = 0; i < fds; i++) {
            p_link = (link_info_t *)events[i].data.ptr;
            if(p_link->fd < 0)
                continue;
            if((events[i].events & EPOLLOUT) && (p_link->events & EPOLLOUT)) { // read event
                if (p_link->tcp_status != SOCKET_CONNECTED)   continue;
                if (p_link->is_in_send_pool != 1) {
                    event_delete(g_epfd_send, p_link); //printf("=====8\n");
                    continue;
                }
                while (1) {
                    int len = send_data(p_link);
                    if (len > 0) {
                        send_count++;
                        continue;
                    }

                    break;
                }
            }
        }
    }
    return NULL;
}

