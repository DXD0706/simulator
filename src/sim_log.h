#ifndef _DCOM_LOG_HXX_
#define _DCOM_LOG_HXX_
#include <stdio.h>
#include "sim_para.h"

extern FILE *fp_log;
extern FILE *fp_net;
extern FILE *fp_poi;
extern FILE *fp_ana;

extern const char *SYS_PATH ;
extern const char *APP_LOG_PATH ;

extern int init_log();
extern int init_msg_log();
extern int log_write_hex(FILE *fp,int direction,unsigned char *buf,size_t len);
extern int log_write_hex(link_info_t *plink_info,int direction,unsigned char *buf,size_t len);

#endif

