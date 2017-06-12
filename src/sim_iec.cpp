#include <time.h>
#include <pthread.h>
#include <math.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>
#include "sim_iec.h"
#include "sim_para.h"
#include "sim_tcp.h"
#include "sim_epoll.h"
#include "sim_log.h"

using namespace std;

int get_current_time(int *year,int *month,int *mday,int *hour,int *min,int *sec,int *msec)
{
    time_t time_now;
    struct tm tm_now;
    struct  timeval tv;
    struct  timezone  tz;

    gettimeofday(&tv,&tz);
    time_now = tv.tv_sec;

    localtime_r(&time_now,&tm_now);

    *year   = tm_now.tm_year+1900;
    *month  = tm_now.tm_mon +1;
    *mday   = tm_now.tm_mday;
    *hour   = tm_now.tm_hour;
    *min    = tm_now.tm_min;
    *sec    = tm_now.tm_sec;
    *msec   = tv.tv_usec /1000;
    return 0;
}


int pack_interrogation_command(unsigned char *buf,size_t len, unsigned short cot, unsigned short common_addr,unsigned char QOI)
{

    if (buf == NULL || len < 10) {
        return -1;
    }

    buf[0] = 0x64;
    buf[1] = 0x01;
    buf[2] = cot % 256;
    buf[3] = cot / 256;
    buf[4] = common_addr % 255;
    buf[5] = common_addr / 255;
    buf[6] = 0x00;
    buf[7] = 0x00;
    buf[8] = 0x00;
    buf[9] = QOI;

    return 10;
}


int pack_counter_interrogation_command(unsigned char *buf,size_t len, unsigned short cot,unsigned short common_addr,unsigned char QCC)
{
    if (buf == NULL || len < 10) {
        return -1;
    }

    buf[0] = 0x65;
    buf[1] = 0x01;
    buf[2] = cot % 256;
    buf[3] = cot / 256;
    buf[4] = common_addr % 256;
    buf[5] = common_addr / 256;
    buf[6] = 0x00;
    buf[7] = 0x00;
    buf[8] = 0x00;
    buf[9] = QCC;
    return 10;
}


int pack_clock_sync_command(unsigned char *buf,size_t len,unsigned short cot,unsigned short common_addr)
{
    int year  = 0;
    int month = 0;
    int mday  = 0;
    int wday  = 0;
    int hour  = 0;
    int min   = 0;
    int sec   = 0;
    int msec  = 0;

    if (buf == NULL || len < 16) {
        return -1;
    }

    get_current_time(&year,&month,&mday,&hour,&min,&sec,&msec);

    buf[0] = 0x67;
    buf[1] = 0x01;
    buf[2] = cot;
    buf[3] = 0x0;
    buf[4] = common_addr % 256;
    buf[5] = common_addr / 256;
    buf[6] = 0;
    buf[7] = 0;
    buf[8] = 0;
    buf[9]  = (sec*1000 + msec) % 256;
    buf[10] = (sec*1000 + msec) / 256;
    buf[11] = min  & 0x3f;
    buf[12] = hour & 0x1f;
    buf[13] = mday&0x1f |((wday << 5)&0xe0);
    buf[14] = month & 0x0f;
    buf[15] = (year - 2000) & 0x7f;
    return 16;
}

int pack_singal_command(unsigned char *buf,size_t len,unsigned short cot,unsigned short common_addr,int index,unsigned char se,unsigned char value)
{
    if (buf == NULL || len < 10) {
        return -1;
    }
	index += 0x6001 - 0x01;
    memset(buf,0x00,len);
    buf[0] = 0x2d;
    buf[1] = 0x01;
    buf[2] = cot % 256;
    buf[3] = cot /256;
    buf[4] = common_addr % 256;
    buf[5] = common_addr / 256;
    buf[6] = index % 256;
    buf[7] = index / 256 % 256;
    buf[8] = index / 65536;
    buf[9] = value & 0x01;

    if (se) {
        buf[9] |= 0x80;
    }

    return 10;
}


int pack_double_command(unsigned char *buf,size_t len,unsigned short cot,unsigned short common_addr,int index,unsigned char se,unsigned char value)
{
    if (buf == NULL || len < 10) {
        return -1;
    }
	index += 0x6001 - 0x01;
    memset(buf,0x00,len);
    buf[0] = 0x2e;
    buf[1] = 0x01;
    buf[2] = cot % 256;
    buf[3] = cot /256;
    buf[4] = common_addr % 256;
    buf[5] = common_addr / 256;
    buf[6] = index % 256;
    buf[7] = index / 256 % 256;
    buf[8] = index / 65536;
    buf[9] = (value & 0x01) + 1;

    if (se) {
        buf[9] |= 0x80;
    }

    return 10;
}


int pack_singal_point_information(unsigned char *buf,int buflen,vector<point_t> &vec_point,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr)
{
    size_t infocount = 50;
    int step = SQ?1:4;

    if ((vec_consumed+infocount) > vec_point.size()) {
        infocount = vec_point.size() - vec_consumed;
    }
    size_t infocount_limit = (buflen - 10) / step + 1;
    if (infocount_limit <= 0) {
        return -1;
    }
    if (infocount_limit < infocount) {
        infocount = infocount_limit;
    }


    buf[0] = 0x01;
    buf[1] = infocount | SQ;
    buf[2] = cot % 256;
    buf[3] = cot / 256;
    buf[4] = common_addr % 256;
    buf[5] = common_addr / 256;
    buf[6] = (vec_point[vec_consumed].index)%256;
    buf[7] = (vec_point[vec_consumed].index)/256;
    buf[8] = 0x00;
    buf[9] = vec_point[vec_consumed].value;
    vec_consumed++;
    for(size_t i=0; i<infocount-1; i++) {
        if (SQ) {
            buf[10+i*step  ] = vec_point[vec_consumed].value;
        } else {
            buf[10+i*step+0] = (vec_point[vec_consumed].index)%256;
            buf[10+i*step+1] = (vec_point[vec_consumed].index)/256;
            buf[10+i*step+2] = 0x00;
            buf[10+i*step+3] = vec_point[vec_consumed].value;
        }
        vec_consumed++;
    }
    return 6 + 4 + (infocount - 1) * step;
}


int pack_double_point_information(unsigned char *buf,int buflen,vector<point_t> &vec_point,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr)
{
    size_t infocount = 50;
    int step = SQ?1:4;

    if ((vec_consumed+infocount) > vec_point.size()) {
        infocount = vec_point.size() - vec_consumed;
    }
    size_t infocount_limit = (buflen - 10) / step + 1;
    if (infocount_limit <= 0) {
        return -1;
    }
    if (infocount_limit < infocount) {
        infocount = infocount_limit;
    }

    buf[0] = 0x03;
    buf[1] = infocount | SQ;
    buf[2] = cot % 256;
    buf[3] = cot / 256;
    buf[4] = common_addr % 256;
    buf[5] = common_addr / 256;
    buf[6] = (vec_point[vec_consumed].index)%256;
    buf[7] = (vec_point[vec_consumed].index)/256;
    buf[8] = 0x00;
    buf[9] = vec_point[vec_consumed].value;

    vec_consumed++;
    for(size_t i=0; i<(infocount-1); i++) {
        if (SQ) {
            buf[10+i*step] = (vec_point[vec_consumed].value & 0x01) + 1;
        } else {
            buf[10+i*step+0] = (vec_point[vec_consumed].index)%256;
            buf[10+i*step+1] = (vec_point[vec_consumed].index)/256;
            buf[10+i*step+2] = 0x00;
            buf[10+i*step+3] = (vec_point[vec_consumed].value & 0x01) + 1;
        }
        vec_consumed++;
    }
    return 6 + 4 + (infocount - 1) * step;
}




int pack_singal_point_information_with_CP56Time2a(unsigned char *buf,int buflen,vector<point_t> &vec_point,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr)
{
    size_t infocount = 50;
    int step = 11;
    int year,month,mday,hour,min,sec,msec;
    SQ = 0;
    if ((vec_consumed+infocount) > vec_point.size()) {
        infocount = vec_point.size() - vec_consumed;
    }
    size_t infocount_limit = (buflen - 6) / step;
    if (infocount_limit <= 0) {
        return -1;
    }
    if (infocount_limit < infocount) {
        infocount = infocount_limit;
    }


    buf[0] = 0x1e;
    buf[1] = infocount;
    buf[2] = cot % 256;
    buf[3] = cot / 256;
    buf[4] = common_addr % 256;
    buf[5] = common_addr / 256;

    for(size_t i=0; i<infocount-1; i++) {
        int index = vec_point[vec_consumed].index;
        buf[6+i*step+0] = index % 256;
        buf[6+i*step+1] = index / 256;
        buf[6+i*step+2] = 0x00;
        buf[6+i*step+3] = vec_point[vec_consumed].value;
        get_current_time(&year,&month,&mday,&hour,&min,&sec,&msec);
        buf[6+i*step+4] = (sec * 1000 + msec) % 256;
        buf[6+i*step+5] = (sec * 1000 + msec) / 256;
        buf[6+i*step+6] = min;
        buf[6+i*step+7] = hour;
        buf[6+i*step+8] = mday;
        buf[6+i*step+9] = month;
        buf[6+i*step+10] = year - 2000;
        vec_consumed++;
    }
    return 6 + infocount * step;
}


int pack_double_point_information_with_CP56Time2a(unsigned char *buf,int buflen,vector<point_t> &vec_point,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr)
{
    size_t infocount = 50;
    int step = 11;
    int year,month,mday,hour,min,sec,msec;
    SQ = 0;
    if ((vec_consumed+infocount) > vec_point.size()) {
        infocount = vec_point.size() - vec_consumed;
    }
    size_t infocount_limit = (buflen - 6) / step;
    if (infocount_limit <= 0) {
        return -1;
    }
    if (infocount_limit < infocount) {
        infocount = infocount_limit;
    }

    buf[0] = 0x1f;
    buf[1] = infocount;
    buf[2] = cot % 256;
    buf[3] = cot / 256;
    buf[4] = common_addr % 256;
    buf[5] = common_addr / 256;

    for(size_t i=0; i<infocount-1; i++) {
        int index = vec_point[vec_consumed].index;
        buf[6+i*step+0] = index % 256;
        buf[6+i*step+1] = index / 256;
        buf[6+i*step+2] = 0x00;
        buf[6+i*step+3] = vec_point[vec_consumed].value + 1;
        get_current_time(&year,&month,&mday,&hour,&min,&sec,&msec);
        buf[6+i*step+4] = (sec * 1000 + msec) % 256;
        buf[6+i*step+5] = (sec * 1000 + msec) / 256;
        buf[6+i*step+6] = min;
        buf[6+i*step+7] = hour;
        buf[6+i*step+8] = mday;
        buf[6+i*step+9] = month;
        buf[6+i*step+10] = year - 2000;
        vec_consumed++;
    }
    return 6 + infocount * step;
}


int pack_measured_value_normalized(unsigned char *buf,int buflen,vector<analog_t> &vec_analog,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr)
{
    size_t infocount = 30;
    int step = SQ?3:6;

    if ((vec_consumed+infocount) > vec_analog.size()) {
        infocount = vec_analog.size() - vec_consumed;
    }
    size_t infocount_limit = (buflen - 6 -6) / step + 1;
    if (infocount_limit <= 0) {
        return -1;
    }
    if (infocount_limit < infocount) {
        infocount = infocount_limit;
    }



    buf[0] = 0x09;
    buf[1] = infocount | SQ;
    buf[2] = cot % 256;
    buf[3] = cot / 256;
    buf[4] = common_addr % 256;
    buf[5] = common_addr / 256;

    int index = vec_analog[vec_consumed].index + 0x4001 - 1;
    buf[6] = index % 256;
    buf[7] = index % 65536 / 256;
    buf[8] = index / 65536;

    int ivalue = (int)vec_analog[vec_consumed].value;

    buf[9]  = ivalue % 256;
    buf[10] = ivalue / 256;
    buf[11] = 0x00;

    vec_consumed++;
    for(size_t i=0; i<infocount-1; i++) {
        int ivalue = (int)vec_analog[vec_consumed].value;

        if (SQ) {
            buf[12+i*3+0] = ivalue % 256;
            buf[12+i*3+1] = ivalue / 256;
            buf[12+i*3+2] = 0x00;
        } else {
            index = vec_analog[vec_consumed].index + 0x4001 - 1;
            buf[12+i*6+0] = index % 256;
            buf[12+i*6+1] = index / 256;
            buf[12+i*6+2] = 0x00;
            buf[12+i*6+3] = ivalue % 256;
            buf[12+i*6+4] = ivalue / 256;
            buf[12+i*6+5] = 0x00;
        }
        vec_consumed++;
    }
    return 6 + 6 + (infocount-1)*step;
}

int pack_measured_value_normalized_without_quality(unsigned char *buf,int buflen,vector<analog_t> &vec_analog,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr)
{
    size_t infocount = 30;
    int step = SQ?2:5;
    if ((vec_consumed+infocount) > vec_analog.size()) {
        infocount = vec_analog.size() - vec_consumed;
    }
    size_t infocount_limit = (buflen - 6 -6) / step + 1;
    if (infocount_limit <= 0) {
        return -1;
    }
    if (infocount_limit < infocount) {
        infocount = infocount_limit;
    }


    buf[0] = 0x15;
    buf[1] = infocount | SQ;
    buf[2] = cot % 256;
    buf[3] = cot / 256;
    buf[4] = common_addr % 256;
    buf[5] = common_addr / 256;

    int index = vec_analog[vec_consumed].index + 0x4001 - 1;
    buf[6] = index % 256;
    buf[7] = index % 65536 / 256;
    buf[8] = index / 65536;

    int ivalue = (int)vec_analog[vec_consumed].value;

    buf[9]  = ivalue % 256;
    buf[10] = ivalue / 256;

    vec_consumed++;
    for(size_t i=0; i<infocount-1; i++) {
        int ivalue = (int)vec_analog[vec_consumed].value;

        if (SQ) {
            buf[11+i*2+0] = ivalue % 256;
            buf[11+i*2+1] = ivalue / 256;
        } else {
            index = vec_analog[vec_consumed].index + 0x4001 - 1;
            buf[11+i*5+0] = index % 256;
            buf[11+i*5+1] = index / 256;
            buf[11+i*5+2] = 0x00;
            buf[11+i*5+3] = ivalue % 256;
            buf[11+i*5+4] = ivalue / 256;
        }
        vec_consumed++;
    }
    return 6 + 5 + (infocount-1)*step;
}

int pack_measured_value_scaled(unsigned char *buf,int buflen,vector<analog_t> &vec_analog,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr)
{
    size_t infocount = 30;
    int step = SQ?3:6;

    if ((vec_consumed+infocount) > vec_analog.size()) {
        infocount = vec_analog.size() - vec_consumed;
    }
    size_t infocount_limit = (buflen - 6 -6) / step + 1;
    if (infocount_limit <= 0) {
        return -1;
    }
    if (infocount_limit < infocount) {
        infocount = infocount_limit;
    }

    buf[0] = 0x0b;
    buf[1] = infocount | SQ;
    buf[2] = cot % 256;
    buf[3] = cot / 256;
    buf[4] = common_addr % 256;
    buf[5] = common_addr / 256;

    int index = vec_analog[vec_consumed].index + 0x4001 - 1;
    buf[6] = index % 256;
    buf[7] = index % 65536 / 256;
    buf[8] = index / 65536;

    int ivalue = (int)vec_analog[vec_consumed].value;

    buf[9]  = ivalue % 256;
    buf[10] = ivalue / 256;
    buf[11] = 0x00;

    vec_consumed++;
    for(size_t i=0; i<infocount-1; i++) {
        int ivalue = (int)vec_analog[vec_consumed].value;

        if (SQ) {
            buf[12+i*3+0] = ivalue % 256;
            buf[12+i*3+1] = ivalue / 256;
            buf[12+i*3+2] = 0x00;
        } else {
            index = vec_analog[vec_consumed].index + 0x4001 - 1;
            buf[12+i*6+0] = index % 256;
            buf[12+i*6+1] = index / 256;
            buf[12+i*6+2] = 0x00;
            buf[12+i*6+3] = ivalue % 256;
            buf[12+i*6+4] = ivalue / 256;
            buf[12+i*6+5] = 0x00;
        }
        vec_consumed++;
    }
    return 6 + 6 + (infocount-1)*step;
}


int pack_measured_value_short_floating(unsigned char *buf,int buflen,vector<analog_t> &vec_analog,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr)
{
    size_t infocount = 30;
    int step = SQ?5:8;

    if ((vec_consumed+infocount) > vec_analog.size()) {
        infocount = vec_analog.size() - vec_consumed;
    }
    size_t infocount_limit = (buflen - 6 -8) / step + 1;
    if (infocount_limit <= 0) {
        return -1;
    }
    if (infocount_limit < infocount) {
        infocount = infocount_limit;
    }

    buf[0] = 0x0d;
    buf[1] = infocount | SQ;
    buf[2] = cot % 256;
    buf[3] = cot / 256;
    buf[4] = common_addr % 256;
    buf[5] = common_addr / 256;

    int index = vec_analog[vec_consumed].index + 0x4001 - 1;
    buf[6] = index % 256;
    buf[7] = index / 256;
    buf[8] = 0x00;

    float fvalue = vec_analog[vec_consumed].value;
    unsigned char *pcvalue = (unsigned char *)&fvalue;

    buf[9]  = pcvalue[0];
    buf[10] = pcvalue[1];
    buf[11] = pcvalue[2];
    buf[12] = pcvalue[3];
    buf[13] = 0x00;

    vec_consumed++;
    for(size_t i=0; i<infocount-1; i++) {
        float fvalue = vec_analog[vec_consumed].value;
        unsigned char *pcvalue = (unsigned char *)&fvalue;

        if (SQ) {
            buf[14+i*5+0] = pcvalue[0];
            buf[14+i*5+1] = pcvalue[1];
            buf[14+i*5+2] = pcvalue[2];
            buf[14+i*5+3] = pcvalue[3];
            buf[14+i*5+4] = 0x00;
        } else {
            index = vec_analog[vec_consumed].index + 0x4001 - 1;
            buf[14+i*8+0] = index % 256;
            buf[14+i*8+1] = index / 256;
            buf[14+i*8+2] = 0x00;
            buf[14+i*8+3] = pcvalue[0];
            buf[14+i*8+4] = pcvalue[1];
            buf[14+i*8+5] = pcvalue[2];
            buf[14+i*8+6] = pcvalue[3];
            buf[14+i*8+7] = 0x00;
        }
        vec_consumed++;
    }
    return 6 + 8 + (infocount-1)*step;
}


int pack_integrated_totals(unsigned char *buf,int buflen,vector<analog_t> &vec_analog,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr)
{
    size_t infocount = 30;
    int step = SQ?5:8;

    if ((vec_consumed+infocount) > vec_analog.size()) {
        infocount = vec_analog.size() - vec_consumed;
    }
    size_t infocount_limit = (buflen - 6 -8) / step + 1;
    if (infocount_limit <= 0) {
        return -1;
    }
    if (infocount_limit < infocount) {
        infocount = infocount_limit;
    }


    buf[0] = 0x0f;
    buf[1] = infocount | SQ;
    buf[2] = cot % 256;
    buf[3] = cot / 256;
    buf[4] = common_addr % 256;
    buf[5] = common_addr / 256;
    buf[6] = vec_analog[vec_consumed].index%256;
    buf[7] = vec_analog[vec_consumed].index/256;
    buf[7] |= 0x40;
    buf[8] = 0x00;

    int ivalue = (int)vec_analog[vec_consumed].value;
    unsigned char *pcvalue = (unsigned char *)&ivalue;

    buf[9] = pcvalue[0];
    buf[10] = pcvalue[1];
    buf[11] = pcvalue[2];
    buf[12] = pcvalue[3];
    buf[13] = 0x00;

    vec_consumed++;
    for(size_t i=0; i<infocount-1; i++) {
        int ivalue = (int)vec_analog[vec_consumed].value;
        unsigned char *pcvalue = (unsigned char *)&ivalue;

        if (SQ) {
            buf[14+i*5+0] = pcvalue[0];
            buf[14+i*5+1] = pcvalue[1];
            buf[14+i*5+2] = pcvalue[2];
            buf[14+i*5+3] = pcvalue[3];
            buf[14+i*5+4] = 0x00;
        } else {
            buf[14+i*8+0] = vec_analog[vec_consumed].index%256;
            buf[14+i*8+1] = vec_analog[vec_consumed].index/256;
            buf[14+i*8+1] |= 0x40;
            buf[14+i*8+2] = 0x00;
            buf[14+i*8+3] = pcvalue[0];
            buf[14+i*8+4] = pcvalue[1];
            buf[14+i*8+5] = pcvalue[2];
            buf[14+i*8+6] = pcvalue[3];
            buf[14+i*8+7] = 0x00;
        }
        vec_consumed++;
    }
    return 6 + 8 + (infocount-1)*step;
}


int pack_point_information(unsigned char *asdu,size_t len,vector<point_t> &vec_point,size_t &vec_consumed,int type,unsigned char SQ,unsigned short cot,unsigned short common_addr)
{
    int n = 0;
    memset(asdu,0x00,1024);
    switch (type) {
        case 0x01:
            n = pack_singal_point_information(asdu,len,vec_point,vec_consumed,SQ,cot,common_addr);
            break;
        case 0x03:
            n = pack_double_point_information(asdu,len,vec_point,vec_consumed,SQ,cot,common_addr);
            break;
        case 0x1e:
            n = pack_singal_point_information_with_CP56Time2a(asdu,len,vec_point,vec_consumed,SQ,cot,common_addr);
            break;
        case 0x1f:
            n = pack_double_point_information_with_CP56Time2a(asdu,len,vec_point,vec_consumed,SQ,cot,common_addr);
            break;
        default:
            return -1;
    }

    return n;
}


int pack_measured_value(unsigned char *asdu,size_t len,vector<analog_t> &vec_analog,size_t &vec_consumed,int type,unsigned char SQ,unsigned short cot,unsigned short common_addr)
{
    int n = 0;
    memset(asdu,0x00,1024);
    switch (type) {
        case 0x09:
            n = pack_measured_value_normalized(asdu,len,vec_analog,vec_consumed,SQ,cot,common_addr);
            break;
        case 0x0b:
            n = pack_measured_value_scaled(asdu,len,vec_analog,vec_consumed,SQ,cot,common_addr);
            break;
        case 0x0d:
            n = pack_measured_value_short_floating(asdu,len,vec_analog,vec_consumed,SQ,cot,common_addr);
            break;
        case 0x0f:
            n = pack_integrated_totals(asdu,len,vec_analog,vec_consumed,SQ,0x03,common_addr);
            break;
		case 0x15:
			n = pack_measured_value_normalized_without_quality(asdu,len,vec_analog,vec_consumed,SQ,cot,common_addr);
        default:
            return -1;
    }

    return n;

}


