#include <time.h>
#include <pthread.h>
#include <math.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>
#include "sim_iec.h"
#include "sim_iec104.h"
#include "sim_para.h"
#include "sim_tcp.h"
#include "sim_epoll.h"
#include "sim_log.h"

using namespace std;



int pack_u_format_frame(unsigned char *buf, size_t len, unsigned char cf1)
{
    if (buf == NULL || len < 4) {
        return -1;
    }
    buf[0] = 0x68;
    buf[1] = 0x04;
    buf[2] = cf1;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    return 6;
}


int pack_s_format_frame(unsigned char *buf, size_t len, unsigned short nr)
{
    if (buf == NULL || len < 4) {
        return -1;
    }
    buf[0] = 0x68;
    buf[1] = 0x04;
    buf[2] = 0x01;
    buf[3] = 0x00;
    buf[4] = nr % 256;
    buf[5] = nr / 256;
    return 6;
}


int send_start_frame_act(link_info_t *plink_info)
{
    unsigned char buf[6];

    plink_info->send_no = 0;
    plink_info->recv_no = 0;

    plink_info->s_idlesse_count = 0x80;
    plink_info->r_idlesse_count = 0;
    plink_info->u_idlesse_count = 0x80;
    plink_info->data_ack_count = 0;
    plink_info->data_ack_timeout_count = 0;
    plink_info->data_timeout_count = 0;

    int len = pack_u_format_frame(buf, 6, STARTDT_ACT);
    if (len < 0) {
        return -1;
    }

    int ret = link_sdata_insert(plink_info, buf, 6, g_send_pool);
    if (ret < 0) {
        return -1;
    }

    return 6;
}


int send_start_frame_con(link_info_t *plink_info)
{
    unsigned char buf[6];

    plink_info->send_no = 0;
    plink_info->recv_no = 0;

    plink_info->s_idlesse_count = 0x80;
    plink_info->r_idlesse_count = 0;
    plink_info->u_idlesse_count = 0x80;
    plink_info->data_ack_count = 0;
    plink_info->data_ack_timeout_count = 0;
    plink_info->data_timeout_count = 0;

    int len = pack_u_format_frame(buf, 6, STARTDT_CON);
    if (len < 0) {
        return -1;
    }

    int ret = link_sdata_insert(plink_info, buf, 6, g_send_pool);
    if (ret < 0) {
        return -1;
    }

    return 6;
}


int send_stop_frame_act(link_info_t *plink_info)
{
    unsigned char buf[6];

    plink_info->send_no = 0;
    plink_info->recv_no = 0;

    plink_info->s_idlesse_count = 0x80;
    plink_info->r_idlesse_count = 0;
    plink_info->u_idlesse_count = 0x80;
    plink_info->data_ack_count = 0;
    plink_info->data_ack_timeout_count = 0;
    plink_info->data_timeout_count = 0;

    int len = pack_u_format_frame(buf, 6, STOPDT_ACT);
    if (len < 0) {
        return -1;
    }

    int ret = link_sdata_insert(plink_info, buf, 6, g_send_pool);
    if (ret < 0) {
        return -1;
    }

    return 6;
}


int send_stop_frame_con(link_info_t *plink_info)
{
    unsigned char buf[6];

    plink_info->send_no = 0;
    plink_info->recv_no = 0;

    plink_info->s_idlesse_count = 0x80;
    plink_info->r_idlesse_count = 0;
    plink_info->u_idlesse_count = 0x80;
    plink_info->data_ack_count = 0;
    plink_info->data_ack_timeout_count = 0;
    plink_info->data_timeout_count = 0;

    int len = pack_u_format_frame(buf, 6, STOPDT_CON);
    if (len < 0) {
        return -1;
    }

    int ret = link_sdata_insert(plink_info, buf, 6, g_send_pool);
    if (ret < 0) {
        return -1;
    }

    return 6;
}


int send_test_frame_act(link_info_t *plink_info)
{
    unsigned char buf[6];

    plink_info->u_idlesse_count = 0;

    int len = pack_u_format_frame(buf, 6, TESTFR_ACT);
    if (len < 0) {
        return -1;
    }

    int ret = link_sdata_insert(plink_info, buf, 6, g_send_pool);
    if (ret < 0) {
        return -1;
    }

    return 6;
}


int send_test_frame_con(link_info_t *plink_info)
{
    unsigned char buf[6];

    plink_info->u_idlesse_count = 0;

    int len = pack_u_format_frame(buf, 6, TESTFR_CON);
    if (len < 0) {
        return -1;
    }

    int ret = link_sdata_insert(plink_info, buf, 6, g_send_pool);
    if (ret < 0) {
        return -1;
    }

    return 6;
}


int send_data_ack(link_info_t *plink_info)
{
    unsigned char buf[6];

    int len = pack_s_format_frame(buf, 6, plink_info->recv_no);
    if (len < 0) {
        return -1;
    }
    int ret = link_sdata_insert(plink_info, buf, 6, g_send_pool);
    if (ret < 0) {
        return -1;
    }

    return 6;
}


int send_i_format_frame(link_info_t *plink_info, unsigned char *asdu, size_t len)
{
    unsigned char buf[len + 6];
    memset(buf, 0x00, sizeof(buf));
    iec104_apci_t *apci = (iec104_apci_t*)buf;

    apci->start_byte = 0x68;
    apci->len = len + 4;
    plink_info->send_no += 2;
    apci->cf1 = plink_info->send_no % 256;
    apci->cf2 = plink_info->send_no / 256;
    apci->cf3 = plink_info->recv_no % 256;
    apci->cf4 = plink_info->recv_no / 256;

    memcpy(&buf[sizeof(iec104_apci_t)], asdu, len);
    int ret = link_sdata_insert(plink_info, buf, len + 6, g_send_pool);
    if (ret < 0) {
        return -1;
    }
    return len + 6;
}


int send_interrogation_command(link_info_t *plink_info, unsigned short cot, unsigned char QOI)
{
    unsigned char buf[10];

    int len = pack_interrogation_command(buf, sizeof(buf), cot, plink_info->rtu_addr, QOI);
    if (len < 0) {
        return len;
    }

    return send_i_format_frame(plink_info, buf, len);
}


int send_counter_interrogation_command(link_info_t *plink_info, unsigned short cot, unsigned char QCC)
{
    unsigned char buf[10];

    int len = pack_counter_interrogation_command(buf, sizeof(buf), cot, plink_info->rtu_addr, QCC);
    if (len < 0) {
        return len;
    }

    return send_i_format_frame(plink_info, buf, len);
}


int send_clock_sync_command(link_info_t *plink_info, unsigned short cot)
{
    unsigned char buf[16];

    int len = pack_clock_sync_command(buf, sizeof(buf), cot, plink_info->rtu_addr);
    if (len < 0) {
        return len;
    }

    return send_i_format_frame(plink_info, buf, len);
    return 1;
}


int send_singal_command(link_info_t *plink_info, unsigned short cot, unsigned short common_addr, int index, unsigned char se, unsigned char value)
{
    unsigned char buf[16];
    printf("send singal command index %d cot %02x value %02x\n", index, cot, value);
    int len = pack_singal_command(buf, sizeof(buf), cot, common_addr, index, se, value);
    if (len < 0) {
        return len;
    }
    return send_i_format_frame(plink_info, buf, len);
}


int send_double_command(link_info_t *plink_info, unsigned short cot, unsigned short common_addr, int index, unsigned char se, unsigned char value)
{
    unsigned char buf[16];
    printf("send double command index %d cot %02x value %02x\n", index, cot, value);
    int len = pack_double_command(buf, sizeof(buf), cot, common_addr, index, se, value);
    if (len < 0) {
        return len;
    }
    return send_i_format_frame(plink_info, buf, len);
}


int analysis_interrogation_command(link_info_t *plink_info, unsigned char *buf/* asdu */, int len)
{
    unsigned char  type = buf[0];
    unsigned char  vsq  = buf[1];
    unsigned short cot  = buf[2] + buf[3] * 256;
    int common_addr = buf[4] + buf[5] * 256;
    int index = buf[6] + buf[7] * 256 + buf[8] * 65536;
    unsigned char QOI = buf[9];

    if (len != 10) {
        return -1;
    }

    if (type != 0x64 || vsq != 0x01) {
        return -1;
    }

    if (cot == 0x06 || cot == 0x08) {
        if (plink_info->proto_status == PROTO_STATUS_INIT) {
            plink_info->rtu_addr = common_addr;
            plink_info->proto_status = PROTO_STATUS_RUN;
        }

        if (common_addr != plink_info->rtu_addr) {
            send_interrogation_command(plink_info, 0x2e | 0x40, QOI);
        } else if (index != 0) {
            send_interrogation_command(plink_info, 0x2f | 0x40, QOI);
        } else {
            if (cot == 0x06) {
                send_interrogation_command(plink_info, 0x07, QOI);
                plink_info->interrogation = 1;
            } else {
                send_interrogation_command(plink_info, 0x09, QOI);
                plink_info->interrogation = 0;
            }
        }
    } else {
        send_interrogation_command(plink_info, 0x2d | 0x40, QOI);
    }
    return 0;
}


int analysis_counter_interrogation_command(link_info_t *plink_info, unsigned char *buf/* asdu */, int len)
{
    unsigned char  type = buf[0];
    unsigned char  vsq  = buf[1];
    unsigned short cot  = buf[2] + buf[3] * 256;
    int common_addr = buf[4] + buf[5] * 256;
    int index = buf[6] + buf[7] * 256 + buf[8] * 65536;
    unsigned char QCC = buf[9];

    if (len != 10) {
        return -1;
    }
    if (type != 0x65 || vsq != 0x01) {
        return -1;
    }

    if (cot == 0x06) {
        if (common_addr != plink_info->rtu_addr) {
            send_counter_interrogation_command(plink_info, 0x2e | 0x40, QCC);
        } else if (index != 0) {
            send_counter_interrogation_command(plink_info, 0x2f | 0x40, QCC);
        } else {
            send_counter_interrogation_command(plink_info, 0x07, QCC);
            plink_info->counter_interrogation = QCC;
        }
    } else {
        send_counter_interrogation_command(plink_info, 0x2d | 0x40, QCC);
    }
    return 0;
}


int analysis_clock_sync_command(link_info_t *plink_info, unsigned char *buf/* asdu */, int len)
{
    unsigned char  type = buf[0];
    unsigned char  vsq  = buf[1];
    unsigned short cot  = buf[2] + buf[3] * 256;
    int common_addr = buf[4] + buf[5] * 256;
    int index = buf[6] + buf[7] * 256 + buf[8] * 65536;

    if (len != 16) {
        return -1;
    }

    if (type != 0x67 || vsq != 0x01) {
        return -1;
    }

    if (common_addr != plink_info->rtu_addr) {
        return -1;
    }

    if (index != 0) {
        return -1;
    }

    if (cot == 0x06) {
        send_clock_sync_command(plink_info, 0x07);
    }
    return 0;
}


int analysis_singal_command(link_info_t *plink_info, unsigned char *buf /* asdu */, int len)
{
    unsigned char type = buf[0];
    unsigned char vsq  = buf[1];
    unsigned short cot = buf[2] + buf[3] * 256;
    unsigned short common_addr = buf[4] + buf[5] * 256;
    int index = buf[6] + buf[7] * 256 + buf[8] * 65536;
    unsigned char sco  = buf[9];
    unsigned char scs  = sco & 0x01;
    unsigned char se   = (sco & 0x80) >> 7;

    if (type != 0x2d || vsq != 0x01) {
        return -1;
    }
    index = index - 0x6001 + 0x01;
    printf("recv singal command index %d cot %02x value %02x %s\n", index, cot, scs, se == 0x01 ? "select" : "exec");
    if (cot != 0x06 && cot != 0x08) {
        send_singal_command(plink_info, 0x2d | 0x40, common_addr, index, se, scs);
        return 0;
    }
    if (common_addr != plink_info->rtu_addr) {
        send_singal_command(plink_info, 0x40 | 0x2e, common_addr, index, se, scs);
        return 0;
    }
    if (index < 1 || index > g_point_count) {
        send_singal_command(plink_info, 0x40 | 0x2f, common_addr, index, se, scs);
        return 0;
    }

    if (cot == 0x06) {
        send_singal_command(plink_info, 0x07, common_addr, index, se, scs);

        if (se == 0) {
            g_raw_point[index].current_value = scs;
        }
    } else {
        send_singal_command(plink_info, 0x09, common_addr, index, se, scs);
    }

    return 0;
}


int analysis_double_command(link_info_t *plink_info, unsigned char *buf/* asdu */, int len)
{
    unsigned char type = buf[0];
    unsigned char vsq  = buf[1];
    unsigned short cot = buf[2] + buf[3] * 256;
    unsigned short common_addr = buf[4] + buf[5] * 256;
    int index = buf[6] + buf[7] * 256 + buf[8] * 65536;
    unsigned char sco  = buf[9];
    unsigned char scs  = (sco & 0x03) - 1;
    unsigned char se   = (sco & 0x80) >> 7;

    if (type != 0x2e || vsq != 0x01) {
        return -1;
    }
    index = index - 0x6001 + 0x01;
    printf("recv double command index %d cot %02x value %02x %s\n", index, cot, scs, se == 0x01 ? "select" : "exec");
    if (cot != 0x06 && cot != 0x08) {
        send_double_command(plink_info, 0x2d | 0x40, common_addr, index, se, scs);
        return 0;
    }
    if (common_addr != plink_info->rtu_addr) {
        send_double_command(plink_info, 0x40 | 0x2e, common_addr, index, se, scs);
        return 0;
    }
    if (index < 1 || index > g_point_count) {
        send_double_command(plink_info, 0x2f | 0x40, common_addr, index, se, scs);
        return 0;
    }
    if (cot == 0x06) {
        send_double_command(plink_info, 0x07, common_addr, index, se, scs);

        if (se == 0) {
            g_raw_point[index].current_value = scs;
        }
    } else {
        send_singal_command(plink_info, 0x09, common_addr, index, se, scs);
    }
    return 0;
}


int analysis_fixed_frame(link_info_t *plink_info, unsigned char *buf, int len)
{
    if (plink_info == NULL || buf == NULL) {
        return -1;
    }

    iec104_apci_t *apci = (iec104_apci_t*)buf;

    switch (apci->cf1) {
        case STARTDT_ACT: //0x07
            send_start_frame_con(plink_info);
            plink_info->tcp_status   = SOCKET_CONNECTED;
            plink_info->proto_status = PROTO_STATUS_INIT;
            break;
        case STARTDT_CON: //0x0b
            plink_info->tcp_status = SOCKET_CONNECTED;
            send_interrogation_command(plink_info, 0x06);
            break;
        case STOPDT_ACT: //0x13
            send_stop_frame_con(plink_info);
            plink_info->tcp_status = SOCKET_CONNECTED;
            break;
        case TESTFR_ACT: //0x43
            send_test_frame_con(plink_info);
            break;
        case TESTFR_CON: //0x83
            plink_info->u_test_send_flag = 0;
            plink_info->u_ack_timeout_count = 0;
            break;
        default:
            break;

    }

    return 0;
}


int analysis_variable_frame(link_info_t *plink_info, unsigned char *buf, int len)
{
    iec104_apci_t *apci = (iec104_apci_t*)buf;
    unsigned char *asdu = &buf[sizeof(iec104_apci_t)];
    unsigned char type  = asdu[0];

    if (apci->start_byte != 0x68) {
        return -1;
    }
    if (apci->len + 2 != len ) {
        return -1;
    }
    int asdu_len = len - sizeof(iec104_apci_t);

    switch(type) {
        case 0x2d:
            analysis_singal_command(plink_info, asdu, asdu_len);
            break;

        case 0x2e:
            analysis_double_command(plink_info, asdu, asdu_len);
            break;

        case 0x64:
            analysis_interrogation_command(plink_info, asdu, asdu_len);
            break;

        case 0x65:
            analysis_counter_interrogation_command(plink_info, asdu, asdu_len);
            break;

        case 0x67:
            analysis_clock_sync_command(plink_info, asdu, asdu_len);
            break;

        default:
            break;
    }

    return 1;
}


int analysis_iec104_frame(link_info_t *plink_info, unsigned char *buf, int len)
{
    if (plink_info == NULL || buf == NULL) {
        return -1;
    }
    if (buf[0] != 0x68) {
        return -1;
    }

    if (len == 6) {
        analysis_fixed_frame(plink_info, buf, len);
    } else {
        analysis_variable_frame(plink_info, buf, len);
    }
    return 0;
}


int send_point_information(link_info_t *plink_info, vector<point_t> &vec_point, int type, unsigned char SQ, unsigned short cot)
{
    size_t offset = 0;
    unsigned char asdu[1024];
    unsigned short common_addr = plink_info->rtu_addr;
    if (g_common_addr >= 0 && g_common_addr <= 65535) {
        common_addr = g_common_addr;
    }
    while(offset < vec_point.size()) {
        int len = 0;
        memset(asdu, 0x00, 1024);
        switch (type) {
            case 0x01:
                len = pack_singal_point_information(asdu, 1024, vec_point, offset, SQ, cot, common_addr);
                break;
            case 0x03:
                len = pack_double_point_information(asdu, 1024, vec_point, offset, SQ, cot, common_addr);
                break;
            case 0x1e:
                len = pack_singal_point_information_with_CP56Time2a(asdu, 1024, vec_point, offset, SQ, cot, common_addr);
                break;
            case 0x1f:
                len = pack_double_point_information_with_CP56Time2a(asdu, 1024, vec_point, offset, SQ, cot, common_addr);
                break;
            default:
                return -1;
        }
        if (len > 0) {
            send_i_format_frame(plink_info, asdu, len);
        }
    }
    return 0;
}


int send_measured_value(link_info_t *plink_info, vector<analog_t> &vec_analog, int type, unsigned char SQ, unsigned short cot)
{
    size_t offset = 0;
    unsigned char asdu[1024];
    unsigned short common_addr = plink_info->rtu_addr;
    if (g_common_addr >= 0 && g_common_addr <= 65535) {
        common_addr = g_common_addr;
    }
    while(offset < vec_analog.size()) {
        int len = 0;
        memset(asdu, 0x00, 1024);
        switch (type) {
            case 0x09:
                len = pack_measured_value_normalized(asdu, 1024, vec_analog, offset, SQ, cot, common_addr);
                break;
            case 0x0b:
                len = pack_measured_value_scaled(asdu, 1024, vec_analog, offset, SQ, cot, common_addr);
                break;
            case 0x0d:
                len = pack_measured_value_short_floating(asdu, 1024, vec_analog, offset, SQ, cot, common_addr);
                break;
            case 0x0f:
                len = pack_integrated_totals(asdu, 1024, vec_analog, offset, SQ, 0x03, common_addr);
                break;
            case 0x15:
                len = pack_measured_value_normalized_without_quality(asdu, 1024, vec_analog, offset, SQ, cot, common_addr);
                break;
            default:
                return -1;
        }
        if (len > 0) {
            send_i_format_frame(plink_info, asdu, len);
        }
    }
    return 0;

}


int send_point_information_changed(link_info_t *plink_info, vector<point_t> &vec_point)
{
    return send_point_information(plink_info, vec_point, g_frame_type_point, 0, 0x03);
}


int send_measured_value_changed(link_info_t *plink_info, vector<analog_t> &vec_analog)
{
    return send_measured_value(plink_info, vec_analog, g_frame_type_analog, 0, 0x03);
}


int send_point_information_total(link_info_t *plink_info, vector<point_t> &vec_point)
{
    return send_point_information(plink_info, vec_point, g_frame_type_point, 0x01 << 7, 0x14);
}


int send_measured_value_total(link_info_t *plink_info, vector<analog_t> &vec_analog)
{
    return send_measured_value(plink_info, vec_analog, g_frame_type_analog, 0x01 << 7, 0x14);
}


void *t_iec104_circle(void *arg)
{
    link_info_t *plink_info;

    log_write_pid();

    while(sig_flag) {
        sleep(1);
        for_each_link(g_linkMap) {
            plink_info = it->second;
            if(plink_info->data_ack_count > 0) {
                plink_info->data_ack_timeout_count++;
            }
            if(plink_info->data_ack_timeout_count >= DATA_ACK_TIMOUT_LIMIT) {
                send_data_ack(plink_info);
                plink_info->data_ack_timeout_count = 0;
                plink_info->data_ack_count = 0;
            }


            if(plink_info->tcp_status == SOCKET_CONNECTED ) {
                plink_info->r_idlesse_count++;
                if(plink_info->r_idlesse_count >= T3_TIMEOUT) {
                    plink_info->u_test_send_flag = 1;
                    plink_info->r_idlesse_count = 0;
                    send_test_frame_act(plink_info);
                }
            }

            if(plink_info->u_test_send_flag > 0 && plink_info->tcp_status == SOCKET_CONNECTED) {
                plink_info->u_ack_timeout_count++;
                if(plink_info->u_ack_timeout_count >= TESTFR_TIMEOUT_LIMIT) {
                    if (plink_info->fp_msg) {
                        fprintf(plink_info->fp_msg, "linkno:%05d wait test frame time out\n", plink_info->linkno);
                        fflush(plink_info->fp_msg);
                    }
                    fprintf(fp_net, "linkno:%05d closed for no test frame reply\n", plink_info->linkno);
                    fflush(fp_net);
                    link_shut(plink_info);
                }
            }
        }
    }
    return NULL;
}


void *t_background_scanning(void *arg)
{
    vector<point_t> vec_change_point;
    vector<analog_t> vec_change_analog;

    log_write_pid();

    while (sig_flag) {
        vec_change_point.clear();
        vec_change_analog.clear();

        for (int i = 1; i <= g_point_count; i++) {
            if (g_raw_point[i].last_value != g_raw_point[i].current_value) {
                point_t tp;
                tp.index = i;
                tp.value = g_raw_point[i].current_value;
                g_raw_point[i].last_value = g_raw_point[i].current_value;
                vec_change_point.push_back(tp);
            }
        }
        for (int i = 1; i <= g_analog_count; i++) {
            if (g_raw_analog[i].last_value != g_raw_analog[i].current_value) {
                analog_t tp;
                tp.index = i;
                tp.value = g_raw_analog[i].current_value;
                g_raw_analog[i].last_value = g_raw_analog[i].current_value;
                vec_change_analog.push_back(tp);
            }
        }
        link_info_t *plink_info  = NULL;
        for_each_link(g_linkMap) {
            plink_info = it->second;
            if(plink_info->proto_status == PROTO_STATUS_RUN && plink_info->interrogation == 0) {
                if (vec_change_point.size() > 0) {
                    send_point_information_changed(plink_info, vec_change_point);
                }
                if (vec_change_analog.size() > 0) {
                    send_measured_value_changed(plink_info, vec_change_analog);
                }
            }
        }
        usleep(1000 * 1000);
    }
    return NULL;
}


void *t_check_interrogation_command(void *arg)
{
    vector<point_t> vec_point;
    vector<analog_t> vec_analog;

    log_write_pid();

    while (sig_flag) {
        vec_point.clear();
        vec_analog.clear();
        for (int i = 1; i < g_point_count; i++) {
            point_t tp;
            tp.index = i;
            tp.value = g_raw_point[i].last_value;
            vec_point.push_back(tp);
        }

        for (int i = 1; i < g_analog_count; i++) {
            analog_t tp;
            tp.index = i;
            tp.value = g_raw_analog[i].last_value;
            vec_analog.push_back(tp);
        }

        link_info_t *plink_info  = NULL;
        for_each_link(g_linkMap) {
            plink_info = it->second;
            if(plink_info->proto_status == PROTO_STATUS_RUN || plink_info->proto_status == PROTO_STATUS_INIT) {
                if (plink_info->interrogation) {
                    send_point_information_total(plink_info, vec_point);
                    send_measured_value_total(plink_info, vec_analog);
                    send_interrogation_command(plink_info, 0x0a);
                    plink_info->interrogation = 0;
                    if (plink_info->proto_status == PROTO_STATUS_INIT) {
                        plink_info->proto_status = PROTO_STATUS_RUN;
                    }
                }
            }
        }
        sleep(1);
    }
    return NULL;
}



void *t_check_counter_interrogation_command(void *arg)
{
    vector<point_t> vec_point;
    vector<analog_t> vec_analog;

    log_write_pid();

    while (sig_flag) {
        vec_point.clear();
        vec_analog.clear();
        for (int i = 1; i < g_point_count; i++) {
            point_t tp;
            tp.index = i;
            tp.value = g_raw_point[i].last_value;
            vec_point.push_back(tp);
        }

        for (int i = 1; i < g_analog_count; i++) {
            analog_t tp;
            tp.index = i;
            tp.value = g_raw_analog[i].last_value;
            vec_analog.push_back(tp);
        }

        link_info_t *plink_info  = NULL;
        for_each_link(g_linkMap) {
            plink_info = it->second;
            if(plink_info->proto_status == PROTO_STATUS_RUN || plink_info->proto_status == PROTO_STATUS_INIT) {
                if (plink_info->counter_interrogation) {
                    unsigned char QCC = plink_info->counter_interrogation;
                    send_measured_value_total(plink_info, vec_analog);
                    send_counter_interrogation_command(plink_info, 0x0a, QCC);
                    plink_info->counter_interrogation = 0;
                }
            }
        }
        sleep(1);
    }
    return NULL;
}


void *t_generate_change_point(void *arg)
{
    log_write_pid();
    int send_count = 0;
    for (int i = 1; i <= g_point_count; i++) {
        g_raw_point[i].current_value = 0;
        g_raw_point[i].last_value    = 0;
    }

    while(sig_flag) {
        for (int i = 1; (i <= g_point_count) && (g_auto_change_flag == 1); i++) {
            unsigned char value = g_raw_point[i].current_value;
            if (g_raw_point[i].current_value == 0x00) {
                g_raw_point[i].current_value = 0x01;
            } else {
                g_raw_point[i].current_value = 0x00;
            }
            time_t time_now = time(NULL);
            fprintf(fp_poi, "point index %d value %02x --> %02x\t%s", i, value, g_raw_point[i].current_value, ctime(&time_now));
            send_count++;
            if (g_point_change_rate > 0 && g_point_change_rate < 30 && g_point_change_rate > g_point_count) {
                if (send_count % (int)g_point_change_rate == 0) {
                    sleep(1);
                }
            } else {
                usleep((long)(1000 * 1000 / g_point_change_rate));
            }
        }
    }

    return NULL;
}


void *t_generate_change_analog(void *arg)
{
    log_write_pid();
    int send_count = 0;
    for (int i = 1; i <= g_analog_count; i++) {
        g_raw_analog[i].current_value = 0;
        g_raw_analog[i].last_value    = 0;
    }
    int speed_mode = fabs(g_analog_change_rate - g_analog_count);

    while(sig_flag) {
        for (int i = 1; (i <= g_analog_count) && (g_auto_change_flag == 1); i++) {
            float value = g_raw_analog[i].current_value;
            g_raw_analog[i].current_value += 1;
            time_t time_now = time(NULL);
            fprintf(fp_ana, "analog index %d value %f --> %f\t%s", i, value, g_raw_analog[i].current_value, ctime(&time_now));
            send_count++;
            if (speed_mode) {
                if (g_analog_change_rate > 0 && g_analog_change_rate < 60 && g_analog_change_rate > g_analog_count) {
                    if (send_count % (int)g_analog_change_rate == 0) {
                        sleep(1);
                    }
                } else {
                    usleep((long)(1000 * 1000 / g_analog_change_rate));
                }
            }
        }
        if (!speed_mode) {
            usleep(1000 * 1000);
        }
    }

    return NULL;
}


void *t_generate_change_analog_quickly(void *arg)
{
    log_write_pid();
    for (int i = 1; i <= g_analog_count; i++) {
        g_raw_analog[i].current_value = 0;
        g_raw_analog[i].last_value    = 0;
    }

    while(sig_flag) {
        for (int i = 1; (i <= g_analog_count) && (g_auto_change_flag == 1); i++) {
            float value = g_raw_analog[i].current_value;
            g_raw_analog[i].current_value += 1;
            time_t time_now = time(NULL);
            fprintf(fp_ana, "analog index %d value %f --> %f\t%s", i, value, g_raw_analog[i].current_value, ctime(&time_now));
        }
        usleep(1000 * 1000);
    }

    return NULL;
}

