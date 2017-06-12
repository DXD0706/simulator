#include <sys/socket.h>
#include "sim_para.h"

pthread_rwlock_t g_listlock;

int g_epfd_listen;
int g_epfd_recv[MAXRECVTHREAD];
int g_epfd_send = 0;

int g_max_link_count = 1;

map<int,link_info_t *> g_linkMap;
set<unsigned short>    g_portSet;


//raw_point_t  g_raw_point[RAW_POINT_COUNT];
//raw_analog_t g_raw_analog[RAW_POINT_COUNT];

raw_point_t  *g_raw_point = NULL;
raw_analog_t *g_raw_analog = NULL;

unsigned char sig_flag = 1;

int g_auto_change_flag  = 1;
int g_frame_type_point  = 0x01;
int g_frame_type_analog = 0x09;
int g_frame_type_soe    = 0;
int g_common_addr       = -1;
int g_point_count  = 2000;
int g_analog_count = 2000;
float g_point_change_rate = 10;
float g_analog_change_rate = 10;

