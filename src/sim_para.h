#ifndef _DCOM_PARA_HXX_
#define _DCOM_PARA_HXX_

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <assert.h>
#include <string>
#include <map>
#include <list>
#include <set>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>

//#include "sim_ringbuffer.h"
#include "sim_mem_pool.h"

using namespace std;

//#define MAXEPOLLSIZE    2048

#define MAX_WAIT_EVT 200
#define MAXRECVTHREAD 10
#define MAXSENDTHREAD 1

#define MAX_ANA_IN_MES 30
#define MAX_POI_IN_MES 40


#define MAXIPCOUNT 4
#define JOBLEN 5

#define DIR_RECV 0
#define DIR_SEND 1
#define DIR_STOP 2
#define DIR_RECV_STOP 3
#define DIR_SEND_STOP 4

#define PROTO_STATUS_UNINIT 0
#define PROTO_STATUS_INIT   1
#define PROTO_STATUS_RUN 2

#define DATALEN     (256)


enum socketStat {
    SOCKET_CREATED,
    SOCKET_CONNECTING,
    SOCKET_CONNECTED,
    SOCKET_CLOSED,
};


typedef struct link_info_t {
    int         linkno;
    int         fd;
    struct timeval conn_time; //connected time
    pthread_mutex_t fdlock;//todo  init
    int events;
    unsigned char net_status;
    unsigned char tcp_status;
    unsigned char proto_status;
    unsigned char status;
    unsigned char s_idlesse_count; // t1 for i frame ( for 104 )
    unsigned char r_idlesse_count; // t3 for i frame ( for 104 )
    unsigned char u_idlesse_count; // t1 for u frame ( for 104 )
    unsigned char data_ack_timeout_count; // t2 for i frame ( for 104 )
    unsigned char data_ack_count; // t2  > 8
    unsigned char data_timeout_count;
    unsigned char u_test_send_flag; //0-not send test frame,1-have send test frame
    unsigned char u_ack_timeout_count;//by dxd   timeout counter for confirm of test frame
    unsigned char is_inpoll;// 1: in epoll wait list, 0 not in
    unsigned char is_recving;
    unsigned char interrogation;
	unsigned char counter_interrogation;
    unsigned char protocol_deal_type;
    unsigned char buff_r[272]; // recv data buffer
    unsigned char buff_s[DATALEN];// send data buffer
    unsigned short len_r;
    unsigned short len_s;
    unsigned short recv_no;
    unsigned short send_no;
    int rtu_addr;
    int test_count;

    struct mem_block_t *send_list;
    pthread_mutex_t send_list_mutex;
    int is_in_send_pool;

    FILE *fp_msg;
} link_info_t;

typedef struct DATA_VALUEF {
    u_char  quality;
    float   rawvalue;
    int     st_no;
    int     index_no;
} DATA_VALUEF;
typedef struct DATA_VALUEP {
    u_char  value   ;
    u_char  quality;
    int     st_no;
    int     index_no;
} DATA_VALUEP;
typedef struct SOCKET_FD_T {
    int fd;
    int count;
} SOCKET_FD_T;
typedef struct MSG_DATAF {
    DATA_VALUEF dataf[MAX_ANA_IN_MES];
} MSG_DATAF;
typedef struct MSG_DATAP {
    DATA_VALUEP datap[MAX_POI_IN_MES];
} MSG_DATAP;

typedef struct raw_point_t {
    unsigned char current_value;
    unsigned char last_value;
} raw_point_t;

typedef struct raw_analog_t {
    float current_value;
    float last_value;
} raw_analog_t;




//#define RAW_POINT_COUNT 10
//#define RAW_ANALOG_COUNT 10

extern int g_epfd_listen;
extern int g_epfd_recv[MAXRECVTHREAD];
extern int g_epfd_send;
extern int g_max_link_count;

extern unsigned char sig_flag;
extern map<int,link_info_t *> g_linkMap;
extern set<unsigned short>g_portSet;

extern pthread_rwlock_t g_listlock;

extern raw_point_t  *g_raw_point;
extern raw_analog_t *g_raw_analog;

extern int g_auto_change_flag;
extern int g_frame_type_point;

extern int g_frame_type_analog;
extern int g_frame_type_soe;
extern int g_common_addr;
extern int   g_point_count;
extern int   g_analog_count;
extern float g_point_change_rate;
extern float g_analog_change_rate;

#define for_each_link(link) for(map<int,link_info_t *>::iterator it = link.begin();it != link.end();it++)

#define gettid() syscall(__NR_gettid)
#define log_write_pid() {pid_t pid = gettid(); fprintf(fp_log,"prodess id:%d\tprocess name:%s\n",pid,__func__);}

#endif
