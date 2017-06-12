#include "sim_thread.h"
#include "sim_tcp.h"
#include "sim_iec104.h"
#include "sim_log.h"
#include "sim_commond.h"


int start_all_threads()
{
    int ret = 0;
    pthread_t   tid;
    
    ret = pthread_create(&tid,0,&t_iec104_circle,NULL);
    if(ret != 0) {
        perror("pthread_create");
    }
    usleep(20*1000);

    ret = pthread_create(&tid,0,t_background_scanning,NULL);
    if(ret != 0) {
        perror("pthread_create");
        exit(1);
    }
    usleep(20*1000);
    ret = pthread_create(&tid,0,t_check_interrogation_command,NULL);
    if(ret != 0) {
        perror("pthread_create");
        exit(1);
    }
    usleep(20*1000);
    for (int i=0; i<MAXSENDTHREAD; i++) {
		long send_thread_id = i;
        ret = pthread_create(&tid,0,&t_send_circle,(void*)send_thread_id);
        if(ret != 0) {
            perror("pthread_create");
            exit(1);
        }
        usleep(20*1000);
    }
    ret = pthread_create(&tid,0,&t_epoll_send_wait_circle,NULL);
    if(ret != 0) {
        perror("pthread_create");
        exit(1);
    }
    usleep(20*1000);
    for (int i=0; i<MAXRECVTHREAD; i++) {
        ret = pthread_create(&tid,0,&t_epoll_recv_wait_circle,(void*)&g_epfd_recv[i]);
        if(ret != 0) {
            perror("pthread_create");
            exit(1);
        }
        usleep(20*1000);
    }

    ret = pthread_create(&tid,0,&t_generate_change_point,NULL);
    if(ret != 0) {
        perror("pthread_create");
        exit(1);
    }
    usleep(20*1000);
    ret = pthread_create(&tid,0,&t_generate_change_analog,NULL);
    if(ret != 0) {
        perror("pthread_create");
        exit(1);
    }

	usleep(20*1000);
    ret = pthread_create(&tid,0,&t_listen_commond,NULL);
    if(ret != 0) {
        perror("pthread_create");
        exit(1);
    }
	
    return 0;
}

