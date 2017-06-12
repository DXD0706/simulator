#include <set>
#include <stdio.h>
#include "sim_para.h"

using namespace std;


int get_conf(char *file,char *key,char *value,size_t len)
{
	char buf[128];
    char *pbuf = buf;
	char buf_head[64];
	char buf_body[64];
	size_t buf_len = sizeof(buf);
	
	if (file == NULL || key == NULL || value == NULL) {
		return -1;
	}
	
	FILE *fp = fopen(file,"r");
    if (fp == NULL) {
        perror("fopen");
		printf("path:%s\n",file);
        return -1;
    }

    while(1) {
		memset(buf,0x00,buf_len);
        if (getline(&pbuf,&buf_len,fp) < 0) {
            break;
		}
        int stringlen = strlen(pbuf);
        if (stringlen < 3) {
			continue;
		}
		if (pbuf[stringlen-1] == '\n') {
			pbuf[stringlen-1] = '\0';
		}
		memset(buf_head,0x00,sizeof(buf_head));
		memset(buf_body,0x00,sizeof(buf_body));
		sscanf(pbuf,"%s = %s",buf_head,buf_body);
		if (strcmp(key,buf_head) == 0) {
			snprintf(value,len,"%s",buf_body);
			fclose(fp);
			return 0;
		}
    }
	fclose(fp);
	return -1;
}

int init_data_scale()
{
	int ret = 0;
	char buf_value[64];
	int count = -1;
	ret = get_conf("conf/sim.conf","point_count",buf_value,sizeof(buf_value));
	if (ret == 0) {
		sscanf(buf_value,"%d",&count);
		if (count >=0 && count < 20000) {
			g_point_count = count;
		}
	} else {
		printf("get_conf point_count\t[fail]\n");
	}
	ret = get_conf("conf/sim.conf","analog_count",buf_value,sizeof(buf_value));
	if (ret == 0) {
		count = -1;
		sscanf(buf_value,"%d",&count);
		if (count >=0 && count < 20000) {
			g_analog_count = count;
		}
	} else {
		printf("get_conf analog_count\t[fail]\n");
	}
	g_raw_point  = (raw_point_t*)  malloc(sizeof(raw_point_t)  * (g_point_count + 1));
	g_raw_analog = (raw_analog_t*) malloc(sizeof(raw_analog_t) * (g_analog_count + 1));
	printf("point  scale %d\n",g_point_count);
	printf("analog scale %d\n",g_analog_count);
	return 0;

}
int init_send_rate()
{
	int ret = 0;
	char buf_value[64];
	float rate = -1;
	
	ret = get_conf("conf/sim.conf","point_rate",buf_value,sizeof(buf_value));
	if (ret == 0) {
		sscanf(buf_value,"%f",&rate);
		if (rate >0 && rate <= 20000) {
			g_point_change_rate = rate;
		}
	} else {
		printf("get_conf point_rate\t[fail]\n");
	}
	ret = get_conf("conf/sim.conf","analog_rate",buf_value,sizeof(buf_value));
	if (ret == 0) {
		rate = -1;
		sscanf(buf_value,"%f",&rate);
		if (rate >0 && rate <= 20000) {
			g_analog_change_rate = rate;
		}
	} else {
		printf("get_conf analog_rate\t[fail]\n");
	}
	printf("point  send rate %0.2f/s\n",g_point_change_rate);
	printf("analog send rate %0.2f/s\n",g_analog_change_rate);
	return 0;
    
}

int init_frame_type()
{
	int ret = 0;
	char buf_value[64];
	unsigned int type = 0x00;
	
	ret = get_conf("conf/sim.conf","analog_type",buf_value,sizeof(buf_value));
	if (ret == 0) {
		sscanf(buf_value,"%x",&type);
		if (type == 0x09 || type == 0x0b || type == 0x0d || type == 0x0f || type == 0x15) {
			g_frame_type_analog = type;
		}
	}
	
	ret = get_conf("conf/sim.conf","point_type",buf_value,sizeof(buf_value));
	if (ret == 0) {
		sscanf(buf_value,"%x",&type);
		if (type == 0x01 || type == 0x03 || type == 0x1e || type == 0x1f) {
			g_frame_type_point = type;
		}
	}
	
	printf("point  frame type 0x%02x\n",g_frame_type_point);
	printf("analog frame type 0x%02x\n",g_frame_type_analog);
	return 0;
    
}

int init_link_count()
{
	int ret = 0;
	char buf_value[64];
	int max_link_count = 0;
	
	ret = get_conf("conf/sim.conf","max_link_count",buf_value,sizeof(buf_value));
	if (ret == 0) {
		sscanf(buf_value,"%d",&max_link_count);
		if ( max_link_count > 0) {
			g_max_link_count = max_link_count;
		}
	}
	printf("max link count %d\n",g_max_link_count);
	return 0;
}

int init_memory_poll()
{
    g_send_pool = mem_pool_create();
    if (g_send_pool == NULL) {
        perror("mem_pool_create");
        exit(1);
    }
    printf("init memory poll\t[ ok ]\n");
    return 0;
}

int init_epoll()
{
    g_epfd_listen = epoll_create(MAX_WAIT_EVT);
    if(g_epfd_listen <= 0) {
        printf("create g_epfd_listen failed.%d/n", g_epfd_listen);
        exit(1);
    }

    for (int i=0; i<MAXRECVTHREAD; i++) {
        g_epfd_recv[i] = epoll_create(MAX_WAIT_EVT);
        if(g_epfd_recv[i] <= 0) {
            printf("create g_epfd_send failed.%d/n", g_epfd_recv[i]);
            exit(1);
        }
    }
    g_epfd_send = epoll_create(MAX_WAIT_EVT);
    if(g_epfd_send <= 0) {
        printf("create g_epfd_send failed.%d/n", g_epfd_send);
        exit(1);
    }
    printf("init epoll\t\t[ ok ]\n");
    return 0;
}
int init_port_listen()
{
    int  port = 0;
    char line_buf[32];
    char *pline_buf = line_buf;
    size_t  line_size = sizeof(line_buf);

    FILE *fp = fopen("conf/listen.conf", "r+");
    if(fp == NULL) {
        printf("open file error:%s\n","conf/listen.conf");
        perror("open");
        printf("use default port 2404\n");
        port = 2404;
        g_portSet.insert(port);
        return 0;
    }

    while(feof(fp) == 0) {
        port = 0;
        memset(line_buf,0x00,sizeof(line_buf));
        int n = getline(&pline_buf,&line_size,fp);
        if (n < 1) {
            continue;
        }
        if (line_buf[0] == '#') {
            continue;
        }
        sscanf(line_buf,"%d",&port);
        if (port > 0 && port < 65536) {
            g_portSet.insert(port);
            //printf("tcp port %d\n",port);
        }
    }
    fclose(fp);

    printf("init port\t\t[ ok ]\n");
    return 0;
}

int init_link()
{
    pthread_rwlock_init(&g_listlock,NULL);

    for(int i=1; i<=g_max_link_count; i++) {

        link_info_t *plink_info = (link_info_t*)malloc(sizeof(link_info_t));
        memset(plink_info,0x00,sizeof(link_info_t));

        plink_info->linkno = i;
        plink_info->fd = -1;
        plink_info->tcp_status   = SOCKET_CLOSED;
        plink_info->proto_status = PROTO_STATUS_UNINIT;
        pthread_mutex_init(&plink_info->send_list_mutex,NULL);
        g_linkMap.insert(pair<int,link_info_t *>(plink_info->linkno,plink_info));
    }
    printf("init link\t\t[ ok ]\n");
    return 0;
}


int init_conf()
{
	init_link_count();
	init_data_scale();
    init_send_rate();	
	init_frame_type();
	return 0;
}
