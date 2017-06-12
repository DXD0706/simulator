#ifndef    _FES_PROTOCOL_104_
#define    _FES_PROTOCOL_104_

#include <stdlib.h>
#include "sim_para.h"

const int IEC104_HEAD = 0x68;

enum BYTE_POS_DEF {
    START_ID = 0,
    APDU_LEN = 1,

    NS       = 2,
    NS_L     = 2,
    NS_H     = 3,
    NR       = 4,
    NR_L     = 4,
    NR_H     = 5,

    APDU_ID  = 6,
    NUM      = 7,

    REASON   = 8,
    REASON_L = 8,
    REASON_H = 9,
    ADDR     = 10,
    ADDR_L   = 10,
    ADDR_H   = 11,

    INDEX_NO = 12,
    INDEX_L  = 12,
    INDEX_M  = 13,
    INDEX_H  = 14,

    VALUE    = 15,
};


enum DATA_LEN_DEF {
    INDEX_LEN     = 3,
    VALUE_LEN_16T = 2,
    VALUE_LEN_32T = 4,

    HEAD_LEN      = 10,
};

//---------------------------------------------------------

enum START_ADDR_DEF {
    DI_START_INDEX              = 0x0001,
    DI_SINGLE_EVENT_START_INDEX = 0x1001,
    DI_START_EVENT_START_INDEX  = 0x2001,
    DI_GROUP_EVENT_START_INDEX  = 0x3001,

    AI_START_INDEX              = 0x4001,
    DC_START_INDEX              = 0x6001,
    SP_START_INDEX              = 0x6201,
    PI_START_INDEX              = 0x6401,
    ONF_START_INDEX             = 0x6601,
};

enum S_TYPE {
    STARTDT_ACT   = 0x04|0x03,    // 0x07
    STARTDT_CON   = 0x08|0x03,    // 0x0b
    STOPDT_ACT    = 0x10|0x03,    // 0x13
    STOPDT_CON    = 0x20|0x03,    // 0x23
    TESTFR_ACT    = 0x40|0x03,    // 0x43
    TESTFR_CON    = 0x80|0x03,    // 0x83

};

#define T3_TIMEOUT 30
#define T1_TIMEOUT 15
const int A_DATA_ACK_104 = 0x0a;
const int DATA_ACK_COUNT_LIMIT = 8;
const int DATA_ACK_TIMOUT_LIMIT = 10;    // 16 -> 14. because some link offen closed by peer
const int TESTFR_TIMEOUT_LIMIT = 15;
const int C_IC_ACT_PERIOD = 60 * 5; //总召唤发送周期(second)
const int RECONNECT_PERION = 2; //重连时间间隔

enum COT_TYPE {
    COT_SPONT        = 3,
    COT_ACT          = 6,
    COT_ACTCON       = 7,
    COT_DEACT        = 8,
    COT_DEACTCON     = 9,
    COT_ACTTERM     = 10,

    COT_INRO         = 20,

    COT_UNKOWN_TYPE  = 44,
    COT_UNKOWN_COT   = 45,
    COT_UNKOWN_ADDR  = 46,
    COT_UNKOWN_INDEX = 47,
};

enum TYPE_IDENTIFICATION {
    SP_NA_1  = 0x01,
    DP_NA_3  = 0x03,
    ME_NA_9  = 0x09,
    ME_NC_13 = 0x0d,

    DC_45    = 0x2d,
    DC_46    = 0x2e,
    PD_47    = 0x2f,
    SP_48    = 0x30,
    SP_49    = 0x31,
    SP_50    = 0x32,

    C_IC_NA_1_100 = 0x64,
    C_CS_NA_1_103 = 0x67,
};

enum DATA_NUM_IN_FRAME {
    SP_NA_1_INTR_NUM   = 127,
    SP_NA_1_SPONT_NUM  = 60,
    ME_NA_9_INTR_NUM   = 80,
    ME_NA_9_SPONT_NUM  = 40,
    ME_NC_13_INTR_NUM  = 48,
    ME_NC_13_SPONT_NUM = 30,
};

enum QUALITY_TYPE {
    DATA_OVERLOAD = 0x01,    // 溢出    0:未溢出    1:溢出
    DATA_BLOCKADE = 0x10,    // 封锁    0:未被封锁  1:被封锁
    DATA_REPLACE  = 0x20,    // 取代    0:未被取代  1:被取代
    DATA_CURRENT  = 0x40,    // 当前值  0:当前值    1:非当前值
    DATA_INVALID  = 0x80,    // 有效    0:有效      1:无效
};

enum QUALITY_TYPE_DEF {
    D_NORMAL    = 0x10,
    D_MANUAL    = 0x20,
    D_OLD       = 0x40,
    D_BAD       = 0x80
};
typedef struct CP56TIME {
    u_char ms[2];
    u_char minute;
    u_char hour;
    u_char day:5;
    u_char wday:3;
    u_char month;
    u_char year;
} CP56TIME;

typedef struct CP24TIME {
    u_char ms[2];
    u_char minute;
} CP24TIME;

typedef    struct    DATA_STRUCT {
    u_char    buf[512];

    u_char    type;    // 0: analog    1: point
    u_char    flag;    // 0: full    1: change
    unsigned short    num;    // data_num
} DATA_STRUCT;

union NSR_FORMAT {
    unsigned short num;
    struct {
        u_char lsb;
        u_char msb;
    };
};

typedef struct I_FORMAT {  // I : Information Transmit Format 编号的信息传输格式
    NSR_FORMAT ns;
    NSR_FORMAT nr;
} I_FORMAT;

typedef struct S_FORMAT {  // S : Numbered Supervisory Function 编号的监视功能格式
    u_char  s_1;
    u_char  s_2;

    NSR_FORMAT nr;
} S_FORMAT;

typedef struct U_FORMAT {  // U : Unnumbered Control Function 不编号的控制功能格式
    u_char  u_1;
    u_char  u_2;
    u_char  u_3;
    u_char  u_4;
} U_FORMAT;

typedef struct C_STRUCT {
    union {
        unsigned short    index_no;
        u_char    buf[2];
    };

    u_char buf3;

    u_char qoi;
} C_STRUCT;

typedef struct SYNC_TIME_STRUCT {
    union {
        unsigned short    index_no;
        u_char    buf[2];
    };

    u_char buf3;

    CP56TIME cp56time;
} SYNC_TIME_STRUCT;

typedef struct P104_ACPI {  // ACPI : Application Protocol Control Information 应用规约控制信息
    u_char    start_id;    // buf[0] 启动字符
    u_char    apdu_len;    // buf[1] APDU的长度

    union {          // buf[2-5] 控制域
        I_FORMAT type_i;
        S_FORMAT type_s;
        U_FORMAT type_u;
        u_char    buf[4];
    };
} P104_ACPI;

typedef struct P104_ASDU {  // ASDU : Application Service Data Unit 应用服务数据单元
    u_char    type;        // buf[6] type identification 类型标识
    u_char    vsq;        // buf[7] variable structure qualifier 可变结构限定词

    union {
        unsigned short    cot;        // buf[8-9] cause of transmission 传送原因
        u_char  cot_buf[2];
    };

    union {
        unsigned short    addr;        // buf[10-11] common address 应用服务数据单元公共地址
        u_char  addr_buf[2];
    };

    union {
        C_STRUCT cs;
        SYNC_TIME_STRUCT sts;
        DATA_STRUCT    data;
    };
} P104_ASDU;

typedef struct P104_APDU {  // APDU : Application Protocol Data Unit 应用规约数据单元
    P104_ACPI    acpi;
    P104_ASDU    asdu;
} P104_APDU;


#define MAX_SOE_IN_MES 20
typedef struct DATA_SOE {
    u_char        value;
    u_char        quality;
    time_t  second;
    unsigned int  msec;
    int           st_no;
    int           index_no;
} DATA_SOE;
typedef struct MSG_SOE {
    DATA_SOE datasoe[MAX_SOE_IN_MES];
} MSG_SOE;

/*********************************************************************/

typedef struct iec104_apci_t {
	unsigned char start_byte;
	unsigned char len;
	unsigned char cf1;
	unsigned char cf2;
	unsigned char cf3;
	unsigned char cf4;
} iec104_apci_t;

/*********************************************************************/
extern int link_shut(link_info_t *plink_info);
int send_interrogation_command(link_info_t *plink_info, unsigned short cot,unsigned char QOI = 0x14);

extern int analysis_iec104_frame(link_info_t *plink_info,unsigned char *buf,int len);
extern void *t_background_scanning(void *p);
extern void *t_check_interrogation_command(void *arg);
extern void *t_generate_change_point(void *arg);
extern void *t_generate_change_analog(void *arg);
extern void *t_generate_change_analog_quickly(void *arg);

#endif

