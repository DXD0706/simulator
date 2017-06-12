#ifndef _SIM_IEC_H_
#define _SIM_IEC_H_

#include <vector>
using namespace std;

typedef struct point_t {
    int index;
    unsigned char value;
} point_t;

typedef struct analog_t {
    int   index;
    float value;
} analog_t;

int pack_interrogation_command(unsigned char *buf,size_t len, unsigned short cot, unsigned short common_addr,unsigned char QOI);
int pack_counter_interrogation_command(unsigned char *buf,size_t len, unsigned short cot,unsigned short common_addr,unsigned char QCC);
int pack_clock_sync_command(unsigned char *buf,size_t len,unsigned short cot,unsigned short common_addr);
int pack_singal_command(unsigned char *buf,size_t len,unsigned short cot,unsigned short common_addr,int index,unsigned char se,unsigned char value);
int pack_double_command(unsigned char *buf,size_t len,unsigned short cot,unsigned short common_addr,int index,unsigned char se,unsigned char value);
int pack_singal_point_information(unsigned char *buf,int buflen,vector<point_t> &vec_point,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr);
int pack_double_point_information(unsigned char *buf,int buflen,vector<point_t> &vec_point,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr);
int pack_singal_point_information_with_CP56Time2a(unsigned char *buf,int buflen,vector<point_t> &vec_point,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr);
int pack_double_point_information_with_CP56Time2a(unsigned char *buf,int buflen,vector<point_t> &vec_point,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr);
int pack_measured_value_normalized(unsigned char *buf,int buflen,vector<analog_t> &vec_analog,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr);
int pack_measured_value_normalized_without_quality(unsigned char *buf,int buflen,vector<analog_t> &vec_analog,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr);
int pack_measured_value_scaled(unsigned char *buf,int buflen,vector<analog_t> &vec_analog,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr);
int pack_measured_value_short_floating(unsigned char *buf,int buflen,vector<analog_t> &vec_analog,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr);
int pack_integrated_totals(unsigned char *buf,int buflen,vector<analog_t> &vec_analog,size_t &vec_consumed,unsigned char SQ,unsigned short cot,unsigned short common_addr);
int pack_point_information(unsigned char *asdu,size_t len,vector<point_t> &vec_point,size_t &vec_consumed,int type,unsigned char SQ,unsigned short cot,unsigned short common_addr);
int pack_measured_value(unsigned char *asdu,size_t len,vector<analog_t> &vec_analog,size_t &vec_consumed,int type,unsigned char SQ,unsigned short cot,unsigned short common_addr);

#endif

