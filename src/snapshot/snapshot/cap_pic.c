#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "cap_pic.h"

mqd_t ipc_mq;

void dump_string(char *source_file, const char *func, int line, char *text, ...)
{
    time_t now;
    struct tm *s_now;
    struct timeval tv;

    if (text == '\0') {
        return;
    }

    time(&now);
    s_now = localtime(&now);
    if (s_now != 0) {
        gettimeofday((struct timeval *) &tv, NULL);
        printf("\n%s(%s-%d)[%02d:%02d:%02d.%03d]:", source_file, func, line, s_now->tm_hour,
                s_now->tm_min, s_now->tm_sec, tv.tv_usec >> 10);
        va_list args;
        va_start (args, text);
        vfprintf(stdout, text, args);
        va_end (args);
        putchar(10);
    }
    return;
}

int mqueue_send(mqd_t mqfd,char *send_buf, size_t send_len)
{
    int iRet;

    if (send_len > 512 || mqfd == -1) {
        return -1;
    }
    iRet = mq_send(mqfd, send_buf, send_len, 1);

    return iRet;
}

int cloud_send_msg(mqd_t mqfd, MSG_TYPE msg_type, char *payload, int payload_len)
{
    cloud_send_msg_ex(mqfd, msg_type, 1, payload, payload_len);
}

int cloud_send_msg_ex(mqd_t mqfd, MSG_TYPE msg_type, short sub, char *payload, int payload_len)
{
    mqueue_msg_header_t MsgHead;
    char send_buf[1024] = {0};
    int send_len = 0;
    int fsMsgRet = 0;

    memset(&MsgHead, 0, sizeof(MsgHead));
    MsgHead.srcMid = MID_CLOUD;
    MsgHead.mainOp = msg_type;
    MsgHead.subOp = sub;
    MsgHead.msgLength = payload_len;

    switch(msg_type)
    {
    case RCD_START_SHORT_VIDEO:
    case RCD_START_SHORT_VIDEO_10S:
    case RCD_START_VOICECMD_VIDEO:
        MsgHead.dstMid = MID_RCD;
        break;
    case RMM_START_CAPTURE:
    case RMM_SPEAK_BAN_DEVICE:
    case RMM_START_PANORAMA_CAPTURE:
    case RMM_ABORT_PANORAMA_CAPTURE:
        MsgHead.dstMid = MID_RMM;
        break;
    case CLOUD_DEBUG_ALARM:
        MsgHead.dstMid = MID_CLOUD;
        break;
    default:
        MsgHead.dstMid = MID_DISPATCH;
        break;
    }

    memcpy(send_buf, &MsgHead, sizeof(MsgHead));
    if(NULL!=payload && payload_len>0)
    {
        memcpy(send_buf + sizeof(MsgHead), payload, payload_len);
    }
    send_len = sizeof(MsgHead) + payload_len;

    fsMsgRet = mqueue_send(mqfd, send_buf, send_len);

    return fsMsgRet;
}

int cloud_cap_pic(char *pic_name)
{
    if(cloud_send_msg(ipc_mq, RMM_START_CAPTURE, pic_name, strlen(pic_name)) < 0)
    {
        dump_string(_F_, _FU_, _L_,  "cloud_cap_pic send_msg fail!\n");
        return -1;
    }
    else
    {
        dump_string(_F_, _FU_, _L_,  "cloud_cap_pic send_msg ok!\n");
        return 0;
    }
}

int main_cloud_cap_pic(char *nameBuff)
{
    ipc_mq = mq_open("/ipc_dispatch", O_RDWR);
    if (ipc_mq == -1) {
        dump_string(_F_, _FU_, _L_, "open queue fail!\n");
        return -1;
    }

    cloud_cap_pic(nameBuff);

    mq_close(ipc_mq);

    return 0;
}
