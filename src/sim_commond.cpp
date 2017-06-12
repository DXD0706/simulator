#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include "sim_para.h"
#include "sim_commond.h"

#define PORT 6767
#define BUFFER_SIZE 4096

extern char *get_current_version();
int analysis_commond_frame(unsigned char *buf, int len,unsigned char *out_buf,int *out_len)
{
    char cmds[10][32];
    int ncmd = 0;
    int npos = 0;
    char *reply_buf =  (char *)out_buf;
    int reply_len = *out_len;
    char last_byte = 0;
    memset(cmds,0x00,sizeof(cmds));
    memset(reply_buf,0x00,reply_len);
    for (int i=0; i<len; i++) {
        if (buf[i] == ' ' || buf[i] == '\t') {
            if (last_byte != ' ' && last_byte != '\t') {
                ncmd++;
            }
            npos = 0;
            last_byte = buf[i];
            continue;
        }
        if (buf[i] == '\n') {
            break;
        }
        cmds[ncmd][npos++] = buf[i];
        last_byte = buf[i];
    }

    if (strcmp(cmds[0],"show") == 0) {
        if (strcmp(cmds[1],"point") == 0) {

            for (int i=1; i<g_point_count; i++) {
                int pos = strlen(reply_buf);
                sprintf(&reply_buf[pos],"index:%3d\tvalue:%02x\n",i,g_raw_point[i].current_value);
            }
        } else if (strcmp(cmds[1],"analog") == 0) {
            for (int i=1; i<g_analog_count; i++) {
                int pos = strlen(reply_buf);
                sprintf(&reply_buf[pos],"index:%3d\tvalue:%f\n",i,g_raw_analog[i].current_value);
            }
        } else if (strcmp(cmds[1],"para") == 0) {
			sprintf(reply_buf,"auto: %d\npoint  type %02xH\nanalog type %02xH\n",g_auto_change_flag,g_frame_type_point,g_frame_type_analog);
            
		} else {
            sprintf(reply_buf,"table name error");
        }
    } else if (strcmp(cmds[0],"set") == 0) {
        if (strcmp(cmds[1],"point") == 0) {
            int index = -1;
            int value = -1;
            sscanf(cmds[2],"%d",&index);
            sscanf(cmds[3],"%d",&value);

            if (index > 0 && index < g_point_count) {
                if (value == 0 || value == 1) {
                    g_raw_point[index].current_value = value;
                } else {
                    sprintf(reply_buf,"value error");
                }
            } else {
                sprintf(reply_buf,"index is out of range");
            }
        } else if (strcmp(cmds[1],"analog") == 0) {
            int   index = -1;
            float value = 0;
            sscanf(cmds[2],"%d",&index);
            sscanf(cmds[3],"%f",&value);

            if (index > 0 && index < g_point_count) {
                g_raw_analog[index].current_value = value;
            } else {
                sprintf(reply_buf,"index is out of range");
            }
        } else if (strcmp(cmds[1],"auto") == 0) {
            int value = -1;
            sscanf(cmds[2],"%d",&value);
            if (value == 0 || value == 1) {
                g_auto_change_flag = value;
                if (value) {
                    sprintf(reply_buf,"auto change on");
                } else {
                    sprintf(reply_buf,"auto change off");
                }
            } else {
                sprintf(reply_buf,"auto value error");
            }
        } else if (strcmp(cmds[1],"type") == 0) {
				if (strcmp(cmds[2],"point") == 0) {
					int value = -1;
            		sscanf(cmds[3],"%d",&value);
					if (value == 0x01 || value == 0x03) {
						g_frame_type_point = value;
					} else {
						sprintf(reply_buf,"type error");
					}
				} else if (strcmp(cmds[2],"analog") == 0) {
					int value = -1;
            		sscanf(cmds[3],"%d",&value);
					if (value == 0x09 || value == 0x0b || value == 0x0d) {
						g_frame_type_analog = value;
					} else {
						sprintf(reply_buf,"type error");
					}
				} else {
					sprintf(reply_buf,"type error");
				}
		} else if (strcmp(cmds[1],"common_addr") == 0) {
			int addr = -1;
			sscanf(cmds[2],"%d",&addr);
			if (addr >= -1 && addr < 65536) {
				g_common_addr = addr;
			} else {
				sprintf(reply_buf,"common addr error");
			}
		} else {
            sprintf(reply_buf,"the second parameter error");
        }
    } else  if (strcmp(cmds[0],"exit") == 0) {
        return 0;
    } else {
        sprintf(reply_buf,"parse error");
    }
	
	reply_len = strlen(reply_buf);
    return reply_len;
}


void *t_recv_commond(void *arg)
{
    int fd = *(int *)arg;
    char buf[BUFFER_SIZE];
    char obuf[BUFFER_SIZE];
    char reply_buf[BUFFER_SIZE];
    int reply_len = BUFFER_SIZE;
    sprintf(buf,"connect successfully, server version %s",get_current_version());
    int vlen = strlen(buf);
    reply_buf[0] = 0xff;
    reply_buf[1] = vlen % 256;
    reply_buf[2] = vlen / 256;
    memcpy(&reply_buf[3],buf,vlen);
    send(fd, reply_buf, vlen + 3, 0);

    while(1) {
        memset(buf, 0x00,BUFFER_SIZE);
        memset(obuf, 0x00,BUFFER_SIZE);
        memset(reply_buf, 0x00,BUFFER_SIZE);

        int n = recv_commond_frame(fd, (unsigned char *)buf, BUFFER_SIZE);
        if (n < 0) {
            close (fd);
            break;
        }
        n = analysis_commond_frame((unsigned char *)buf,n,(unsigned char *)obuf,&reply_len);
        if (n <= 0) {
            close(fd);
            break;
        }
        int rlen = strlen(obuf);
        reply_buf[0] = 0xff;
        reply_buf[1] = rlen % 256;
        reply_buf[2] = rlen / 256;
        memcpy(&reply_buf[3],obuf,rlen);
        send(fd, reply_buf, rlen + 3, 0);
    }
	return NULL;
}


void *t_listen_commond(void *arg)
{
    int op = 1;
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        exit(1);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    if (bind(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr))) {
        perror("bind");
        exit(1);
    }

    if (listen(server_socket, 20)) {
        perror("listen");
        exit(1);
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);

        int new_server_socket = accept(server_socket,(struct sockaddr*) &client_addr, &length);
        if (new_server_socket < 0) {
            perror("accept");
            break;
        }
        pthread_t tid;
        int *psock = (int*)malloc(sizeof(int));
        *psock = new_server_socket;
        pthread_create(&tid,NULL,t_recv_commond,psock);
    }
	return NULL;
}

int recv_commond_frame(int fd, unsigned char *buf, int len)
{
    unsigned head = 0;
    unsigned blen_L = 0;
    unsigned blen_H = 0;

    while(1) {
        int n = recv(fd, &head, 1, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return -2;
        }
        if (head == 0xff) {
            break;
        }
    }

    while(1) {
        int n = recv(fd, &blen_L, 1, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return -2;
        }
        break;
    }

    while(1) {
        int n = recv(fd, &blen_H, 1, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return -2;
        }
        break;
    }

    int nlen = blen_L + blen_H * 256;
    int nread = 0;

    if (nlen > len) {
        return -3;
    }
    while(1) {
        int n = recv(fd, &buf[nread], nlen - nread, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return -2;
        }
        nread += n;
        if (nread == nlen) {
            break;
        }
    }

    return nread;

}


