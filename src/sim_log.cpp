#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include "sim_log.h"
#include "sim_para.h"

FILE *fp_log = NULL;
FILE *fp_net = NULL;
FILE *fp_poi = NULL;
FILE *fp_ana = NULL;

const char *SYS_PATH = ".";
const char *APP_LOG_PATH = "log";
const char *APP_MSG_PATH = "msg";
const char *APP_SEC_PATH = ".";

int g_sec_dev_init = 0;

int create_dir(const char *dir_path)
{
    int ret = 0;
    char path[200],path_t[200];
    char *ptr_start,*ptr_end;
    bzero(path,sizeof(path));
    strncpy(path,dir_path,strlen(dir_path));
    ptr_start   = &path[0];
    ptr_end     = &path[0];
    ret = mkdir(path,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
    if(ret == 0) {
        printf("Succes to Create %s\n",path);
        return 0;
    } else {
        if(errno == EEXIST) { /* File exists                  */
            return 0;
        } else if(errno == ENOENT) { /* No such file or directory    */
        } else {
            return -1;
        }
    }
    while(*ptr_end++ != '\0') {
        if(ptr_end == &path[strlen(path)-1]) {
            ret = mkdir(path,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
            if(ret == 0) {
                printf("Succes to Create %s\n",path);
                return 0;
            } else {
                return -1;
            }
        }
        if(*ptr_end == '/') {
            bzero(path_t,sizeof(path_t));
            strncpy(path_t,path,ptr_end-ptr_start);
            ret = mkdir(path_t,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
            if(ret == 0) {
                continue;
            } else {
                if(errno == EEXIST) { /* File exists                  */
                    continue;
                } else if(errno == EACCES) { /* Permission denied            */
                    printf("%s\n",path_t);
                    perror("mkdir ");
                    return -1;
                } else {
                    return -1;
                }
            }
        }
    }

    return 0;
}


int split_time(int *year,int *month,int *mday)
{
    struct tm tm_now;
    time_t time_now = time(NULL);
    localtime_r(&time_now,&tm_now);

    *year   = tm_now.tm_year+1900;
    *month  = tm_now.tm_mon +1;
    *mday    = tm_now.tm_mday;
    return 0;
}


FILE* create_log_file(char *dir,char *file,int year,int month,int mday)
{
    char filename[256];
    struct stat fstat;
    char o_mode[16] = "a+";
    time_t time_now = time(NULL);

    memset(&fstat,0x00,sizeof(fstat));
    memset(filename,0x00,sizeof(filename));

    create_dir(dir);
    sprintf(filename,"%s/%d-%02d-%02d_%s",dir,year,month,mday,file);
    int ret = stat(filename,&fstat);
    if(ret != -1) {
        struct tm tm_file = *localtime(&fstat.st_ctime);
        int mday_file = tm_file.tm_mday;
        if(mday != mday_file)
            sprintf(o_mode,"w+");
        else
            sprintf(o_mode,"a+");
    }

    FILE *fp = fopen(filename,o_mode);
    if(fp == NULL) {
        printf("cannot open %s , %s\n",filename,strerror(errno));
        return NULL;
    }
    setbuf(fp,NULL);
    fprintf(fp,"\t\t process start : %s",ctime(&time_now));

    return fp;
}

int init_log()
{
    int year  = 0;
    int month = 0;
    int day   = 0;
    char dir_path[255];

    memset(dir_path,0x00,sizeof(dir_path));
    sprintf(dir_path,"%s/%s",SYS_PATH,APP_LOG_PATH);

    split_time(&year,&month,&day);
    fp_log = create_log_file(dir_path,"sim_iec104.log",year,month,day);
    fp_net = create_log_file(dir_path,"sim_iec104.net",year,month,day);
	fp_poi  = create_log_file(dir_path,"sim_iec104.poi",year,month,day);
    fp_ana = create_log_file(dir_path,"sim_iec104.ana",year,month,day);
    if (fp_log == NULL || fp_net == NULL) {
        printf("init log failed\n");
        exit(1);
    }
    return 0;

}

int init_msg_log()
{
    char dir_path[255];
    char filename[256];
    link_info_t *p_buf = NULL;

    memset(dir_path,0x00,sizeof(dir_path));
    sprintf(dir_path,"%s/%s",SYS_PATH,APP_MSG_PATH);

    for_each_link(g_linkMap) {
        p_buf = it->second;
        sprintf(filename,"msg/%d.log",p_buf->linkno);
        p_buf->fp_msg= fopen(filename,"a+");
		if (p_buf->fp_msg == NULL) {
			perror(filename);
		}
    }
    return 0;
}

int log_write_hex(FILE *fp,int direction,unsigned char *buf,size_t len)
{
    char str[1024];
    if (fp == NULL || buf == NULL) {
        return -1;
    }

    time_t time_now = time(NULL);
    memset(str,0x00,sizeof(str));

    if (direction == DIR_RECV) {
        sprintf(str,"RECV %s",ctime(&time_now));
    } else {
        sprintf(str,"SEND %s",ctime(&time_now));
    }

    char *p = &str[strlen(str)];

    for (size_t i=0; i<len; i++) {
        sprintf(p,"%02x ",buf[i]);
        p+=3;
    }

    sprintf(p,"\n");
    fprintf(fp,"%s",str);
    fflush(fp);
    return 0;
}

int log_write_hex(link_info_t *plink_info,int direction,unsigned char *buf,size_t len)
{
	if (plink_info == NULL || buf == NULL) {
		return -1;
	}
	if (plink_info->linkno > 100) {
		return 0;
	}
	return log_write_hex(plink_info->fp_msg, direction, buf, len);
}


