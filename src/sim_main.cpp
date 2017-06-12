#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "sim_para.h"
#include "sim_tcp.h"
#include "sim_thread.h"
#include "sim_log.h"
#include "sim_init.h"

mem_pool_t *g_send_pool = NULL;

char g_version[][32] = {"2.3.3 2015-10-21"};

char *get_current_version()
{
    return g_version[sizeof(g_version) / 32 - 1];
}


void sigchld_handler(int s)
{
    sig_flag = 0;
    return;
}

int main(int argc, char **argv)
{
    struct epoll_event events[MAX_WAIT_EVT];
    signal(SIGINT, sigchld_handler);
    signal(SIGTERM, sigchld_handler);
    signal(SIGPIPE, SIG_IGN);

    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        printf("version %s\n", get_current_version());
        return 0;
    }
	printf("version %s\n", get_current_version());
	
    init_log();
    log_write_pid();
    init_conf();
    init_epoll();
    init_memory_poll();
    init_link();
    init_port_listen();
    listen_ports();
    //init_msg_log();

    start_all_threads();

    while(sig_flag) {
        int n = epoll_wait(g_epfd_listen, events, MAX_WAIT_EVT, 5000);
        if(n < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }
        for(int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            if(events[i].events & EPOLLIN) {
                accept_connection(fd);
            }
        }
    }
    return 0;
}
