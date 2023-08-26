/*
 * Copyright (c) 2023 roleo.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Read the last h264 i-frame from the buffer and convert it using libavcodec
 * and libjpeg.
 * The position of the frame is written in /tmp/iframe.idx
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <dirent.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <getopt.h>
#include <signal.h>

#ifdef HAVE_AV_CONFIG_H
#undef HAVE_AV_CONFIG_H
#endif

#include "libavcodec/avcodec.h"
#include "libavutil/common.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"

#include "convert2jpg.h"
#include "add_water.h"

#define FF_INPUT_BUFFER_PADDING_SIZE 32

#define PATH_RES_HIGH "/tmp/sd/yi-hack-v5/etc/wm_res/high/wm_540p_"
#define PATH_RES_LOW  "/tmp/sd/yi-hack-v5/etc/wm_res/low/wm_540p_"

// yi_home
#define TABLE_HIGH_OFFSET_YI_HOME 0x10
#define TABLE_LOW_OFFSET_YI_HOME 0x12E0
#define TABLE_RECORD_SIZE_YI_HOME 32
#define TABLE_RECORD_NUM_YI_HOME 150
#define BUF_SIZE_YI_HOME 648000
#define STREAM_HIGH_OFFSET_YI_HOME 0x4B40
#define STREAM_LOW_OFFSET_YI_HOME 0x68B40
#define FRAME_COUNTER_OFFSET_YI_HOME 18
#define FRAME_OFFSET_OFFSET_YI_HOME 4
#define FRAME_LENGTH_OFFSET_YI_HOME 8
#define FRAME_TYPE_OFFSET_YI_HOME 16
#define W_LOW_YI_HOME 640
#define H_LOW_YI_HOME 360
#define W_HIGH_YI_HOME 1280
#define H_HIGH_YI_HOME 720

// yi_home_1080p
#define TABLE_HIGH_OFFSET_YI_HOME_1080P 0x10
#define TABLE_LOW_OFFSET_YI_HOME_1080P 0x25A0
#define TABLE_RECORD_SIZE_YI_HOME_1080P 32
#define TABLE_RECORD_NUM_YI_HOME_1080P 300
#define BUF_SIZE_YI_HOME_1080P 1586752
#define STREAM_HIGH_OFFSET_YI_HOME_1080P 0x9640
#define STREAM_LOW_OFFSET_YI_HOME_1080P 0x109640
#define FRAME_COUNTER_OFFSET_YI_HOME_1080P 18
#define FRAME_OFFSET_OFFSET_YI_HOME_1080P 4
#define FRAME_LENGTH_OFFSET_YI_HOME_1080P 8
#define FRAME_TYPE_OFFSET_YI_HOME_1080P 16
#define W_LOW_YI_HOME_1080P 640
#define H_LOW_YI_HOME_1080P 360
#define W_HIGH_YI_HOME_1080P 1920
#define H_HIGH_YI_HOME_1080P 1080

// yi_dome_1080p
#define TABLE_HIGH_OFFSET_YI_DOME_1080P 0x10
#define TABLE_LOW_OFFSET_YI_DOME_1080P 0x25A0
#define TABLE_RECORD_SIZE_YI_DOME_1080P 32
#define TABLE_RECORD_NUM_YI_DOME_1080P 300
#define BUF_SIZE_YI_DOME_1080P 1586752
#define STREAM_HIGH_OFFSET_YI_DOME_1080P 0x9640
#define STREAM_LOW_OFFSET_YI_DOME_1080P 0x109640
#define FRAME_COUNTER_OFFSET_YI_DOME_1080P 18
#define FRAME_OFFSET_OFFSET_YI_DOME_1080P 4
#define FRAME_LENGTH_OFFSET_YI_DOME_1080P 8
#define FRAME_TYPE_OFFSET_YI_DOME_1080P 16
#define W_LOW_YI_DOME_1080P 640
#define H_LOW_YI_DOME_1080P 360
#define W_HIGH_YI_DOME_1080P 1920
#define H_HIGH_YI_DOME_1080P 1080

// yi_cloud_dome_1080p
#define TABLE_HIGH_OFFSET_YI_CLOUD_DOME_1080P 0x10
#define TABLE_LOW_OFFSET_YI_CLOUD_DOME_1080P 0x25A0
#define TABLE_RECORD_SIZE_YI_CLOUD_DOME_1080P 32
#define TABLE_RECORD_NUM_YI_CLOUD_DOME_1080P 300
#define BUF_SIZE_YI_CLOUD_DOME_1080P 1586752
#define STREAM_HIGH_OFFSET_YI_CLOUD_DOME_1080P 0x9640
#define STREAM_LOW_OFFSET_YI_CLOUD_DOME_1080P 0x109640
#define FRAME_COUNTER_OFFSET_YI_CLOUD_DOME_1080P 18
#define FRAME_OFFSET_OFFSET_YI_CLOUD_DOME_1080P 4
#define FRAME_LENGTH_OFFSET_YI_CLOUD_DOME_1080P 8
#define FRAME_TYPE_OFFSET_YI_CLOUD_DOME_1080P 16
#define W_LOW_YI_CLOUD_DOME_1080P 640
#define H_LOW_YI_CLOUD_DOME_1080P 360
#define W_HIGH_YI_CLOUD_DOME_1080P 1920
#define H_HIGH_YI_CLOUD_DOME_1080P 1080

// yi_dome
#define TABLE_HIGH_OFFSET_YI_DOME 0x10
#define TABLE_LOW_OFFSET_YI_DOME 0x1920
#define TABLE_RECORD_SIZE_YI_DOME 32
#define TABLE_RECORD_NUM_YI_DOME 200
#define BUF_SIZE_YI_DOME 654400
#define STREAM_HIGH_OFFSET_YI_DOME 0x6440
#define STREAM_LOW_OFFSET_YI_DOME 0x6A440
#define FRAME_COUNTER_OFFSET_YI_DOME 18
#define FRAME_OFFSET_OFFSET_YI_DOME 4
#define FRAME_LENGTH_OFFSET_YI_DOME 8
#define FRAME_TYPE_OFFSET_YI_DOME 16
#define W_LOW_YI_DOME 640
#define H_LOW_YI_DOME 360
#define W_HIGH_YI_DOME 1280
#define H_HIGH_YI_DOME 720

// yi_outdoor
#define TABLE_HIGH_OFFSET_YI_OUTDOOR 0x10
#define TABLE_LOW_OFFSET_YI_OUTDOOR 0x25A0
#define TABLE_RECORD_SIZE_YI_OUTDOOR 32
#define TABLE_RECORD_NUM_YI_OUTDOOR 300
#define BUF_SIZE_YI_OUTDOOR 1586752
#define STREAM_HIGH_OFFSET_YI_OUTDOOR 0x9640
#define STREAM_LOW_OFFSET_YI_OUTDOOR 0x109640
#define FRAME_COUNTER_OFFSET_YI_OUTDOOR 18
#define FRAME_OFFSET_OFFSET_YI_OUTDOOR 4
#define FRAME_LENGTH_OFFSET_YI_OUTDOOR 8
#define FRAME_TYPE_OFFSET_YI_OUTDOOR 16
#define W_LOW_YI_OUTDOOR 640
#define H_LOW_YI_OUTDOOR 360
#define W_HIGH_YI_OUTDOOR 1920
#define H_HIGH_YI_OUTDOOR 1080

#define TIMEOUT_INTERVAL 5
#define MILLIS_10 10000

#define RESOLUTION_NONE 0
#define RESOLUTION_LOW  360
#define RESOLUTION_HIGH 1080

#define BUFFER_FILE "/tmp/view"

typedef struct {
    int sps_addr;
    int sps_len;
    int pps_addr;
    int pps_len;
    int idr_addr;
    int idr_len;
} frame;

int debug = 0;
unsigned char *addr;

long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds

    return milliseconds;
}

int frame_decode(unsigned char *outbuffer, unsigned char *p, int length)
{
    const AVCodec *codec;
    AVCodecContext *c= NULL;
    AVFrame *picture;
    int got_picture, len;
    FILE *fOut;
    uint8_t *inbuf;
    AVPacket avpkt;
    int i, j, size;

//////////////////////////////////////////////////////////
//                    Reading H264                      //
//////////////////////////////////////////////////////////

    if (debug) fprintf(stderr, "Starting decode\n");

    av_init_packet(&avpkt);

    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        if (debug) fprintf(stderr, "Codec h264 not found\n");
        return -2;
    }

    c = avcodec_alloc_context3(codec);
    picture = av_frame_alloc();

    // Removed check for truncated capabilities and flag assignment

    if (avcodec_open2(c, codec, NULL) < 0) {
        if (debug) fprintf(stderr, "Could not open codec h264\n");
        av_free(c);
        return -2;
    }


    // inbuf is already allocated in the main function
    inbuf = p;
    memset(inbuf + length, 0, FF_INPUT_BUFFER_PADDING_SIZE);

    // Get only 1 frame
    memcpy(inbuf, p, length);
    avpkt.size = length;
    avpkt.data = inbuf;

    // Decode frame
    if (debug) fprintf(stderr, "Decode frame\n");
    if (c->codec_type == AVMEDIA_TYPE_VIDEO ||
         c->codec_type == AVMEDIA_TYPE_AUDIO) {

        len = avcodec_send_packet(c, &avpkt);
        if (len < 0 && len != AVERROR(EAGAIN) && len != AVERROR_EOF) {
            if (debug) fprintf(stderr, "Error decoding frame\n");
            return -2;
        } else {
            if (len >= 0)
                avpkt.size = 0;
            len = avcodec_receive_frame(c, picture);
            if (len >= 0)
                got_picture = 1;
        }
    }
    if(!got_picture) {
        if (debug) fprintf(stderr, "No input frame\n");
        av_frame_free(&picture);
        avcodec_close(c);
        av_free(c);
        return -2;
    }

    if (debug) fprintf(stderr, "Writing yuv buffer\n");
    memset(outbuffer, 0x80, c->width * c->height * 3 / 2);
    memcpy(outbuffer, picture->data[0], c->width * c->height);
    for(i=0; i<c->height/2; i++) {
        for(j=0; j<c->width/2; j++) {
            outbuffer[c->width * c->height + c->width * i +  2 * j] = *(picture->data[1] + i * picture->linesize[1] + j);
            outbuffer[c->width * c->height + c->width * i +  2 * j + 1] = *(picture->data[2] + i * picture->linesize[2] + j);
        }
    }

    // Clean memory
    if (debug) fprintf(stderr, "Cleaning ffmpeg memory\n");
    av_frame_free(&picture);
    avcodec_close(c);
    av_free(c);

    return 0;
}


int add_watermark(char *buffer, int width, int height)
{
    int w_res, h_res;
    char path_res[1024];
    FILE *fBuf;
    WaterMarkInfo WM_info;

    w_res = width;
    h_res = height;
    if (width == 640) {
        strcpy(path_res, PATH_RES_LOW);
    } else if (width == 1280) {
        strcpy(path_res, PATH_RES_LOW);
    } else {
        strcpy(path_res, PATH_RES_HIGH);
    }

    if (WMInit(&WM_info, path_res) < 0) {
        fprintf(stderr, "water mark init error\n");
        free(buffer);
        return -1;
    } else {
        if (width == 640) {
            AddWM(&WM_info, width, height, buffer,
                buffer + width*height, width-230, height-20, NULL);
        } else if (width == 1280) {
            AddWM(&WM_info, w_res, h_res, buffer,
                buffer + width*height, width-345, height-30, NULL);
        } else {
            AddWM(&WM_info, w_res, h_res, buffer,
                buffer + width*height, width-460, height-40, NULL);
        }
        WMRelease(&WM_info);
    }

    return 0;
}

pid_t proc_find(const char* process_name, pid_t process_pid)
{
    DIR* dir;
    struct dirent* ent;
    char* endptr;
    char buf[512];

    if (!(dir = opendir("/proc"))) {
        perror("can't open /proc");
        return -1;
    }

    while((ent = readdir(dir)) != NULL) {
        /* if endptr is not a null character, the directory is not
         * entirely numeric, so ignore it */
        long lpid = strtol(ent->d_name, &endptr, 10);
        if (*endptr != '\0') {
            continue;
        }

        /* try to open the cmdline file */
        snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);
        FILE* fp = fopen(buf, "r");

        if (fp) {
            if (fgets(buf, sizeof(buf), fp) != NULL) {
                /* check the first token in the file, the program name */
                char* first = strtok(buf, " ");
                if ((strcmp(first, process_name) == 0) && ((pid_t) lpid != process_pid)) {
                    fclose(fp);
                    closedir(dir);
                    return (pid_t) lpid;
                }
            }
            fclose(fp);
        }

    }

    closedir(dir);
    return -1;
}

void print_usage(char *prog_name)
{
    fprintf(stderr, "Usage: %s [options]\n", prog_name);
    fprintf(stderr, "\t-r, --res RES                    Set resolution: \"low\" or \"high\" (default \"high\")\n");
    fprintf(stderr, "\t    --width WIDTH                Set width in pixel (alternative to res)\n");
    fprintf(stderr, "\t    --height HIGHT               Set height in pixel (alternative to res)\n");
    fprintf(stderr, "\t-m, --model MODEL                Select cam model: yi_home, yi_home_1080p, yi_dome_1080p, yi_cloud_dome_1080p, yi_dome or yi_outdoor\n");
    fprintf(stderr, "\t    --table_offset VAL           Set the offset of the table for the resolution selected\n");
    fprintf(stderr, "\t    --table_record_size VAL      Set the size of the record in the table\n");
    fprintf(stderr, "\t    --table_record_num VAL       Set the number of record in the table\n");
    fprintf(stderr, "\t    --buf_size VAL               Set the size of the buffer file\n");
    fprintf(stderr, "\t    --stream_offset VAL          Set the offset of the stream for the resolution selected\n");
    fprintf(stderr, "\t    --frame_counter_offset VAL   Set the offset of the frame counter in the record\n");
    fprintf(stderr, "\t    --frame_offset_offset VAL    Set the offset of the frame offset in the record\n");
    fprintf(stderr, "\t    --frame_length_offset VAL    Set the offset of the frame lenght in the record\n");
    fprintf(stderr, "\t    --frame_type_offset VAL      Set the offset of the frame type in the record\n");
    fprintf(stderr, "\t-w, --watermark                  Add watermark to image\n");
    fprintf(stderr, "\t-d, --debug                      Enable debug\n");
    fprintf(stderr, "\t-h, --help                       Show this help\n");
}

int main(int argc, char **argv) {
    unsigned char *frame_ptr;
    unsigned int frame_offset;
    unsigned int frame_length;
    unsigned char *record_ptr, *next_record_ptr;

    FILE *fFid = NULL;

    int current_frame, frame_counter, frame_type, frame_counter_tmp, next_frame_counter, next_frame_type;
    int frame_type_sum;
    int table_offset, stream_offset;

    int i, c, i_tmp;
    mode_t mode = 0755;
    char *endptr;

    int table_high_offset;
    int table_low_offset;
    int table_record_size;
    int table_record_num;
    int buf_size;
    int stream_high_offset;
    int stream_low_offset;
    int frame_counter_offset;
    int frame_offset_offset;
    int frame_length_offset;
    int frame_type_offset;
    int width, height;
    int w_low, h_low, w_high, h_high;

    unsigned char *addr;
    int resolution = RESOLUTION_HIGH;
    int watermark = 0;

    unsigned char *bufferh264, *bufferyuv;
    int bufferh264_size;

    time_t start_time, current_time;

    // Settings default
    table_high_offset = TABLE_HIGH_OFFSET_YI_HOME_1080P;
    table_low_offset = TABLE_LOW_OFFSET_YI_HOME_1080P;
    table_record_size = TABLE_RECORD_SIZE_YI_HOME_1080P;
    table_record_num = TABLE_RECORD_NUM_YI_HOME_1080P;
    buf_size = BUF_SIZE_YI_HOME_1080P;
    stream_high_offset = STREAM_HIGH_OFFSET_YI_HOME_1080P;
    stream_low_offset = STREAM_LOW_OFFSET_YI_HOME_1080P;
    frame_counter_offset = FRAME_COUNTER_OFFSET_YI_HOME_1080P;
    frame_offset_offset = FRAME_OFFSET_OFFSET_YI_HOME_1080P;
    frame_length_offset = FRAME_LENGTH_OFFSET_YI_HOME_1080P;
    frame_type_offset = FRAME_TYPE_OFFSET_YI_HOME_1080P;
    w_low = W_LOW_YI_HOME_1080P;
    h_low = H_LOW_YI_HOME_1080P;
    w_high = W_HIGH_YI_HOME_1080P;
    h_high = H_HIGH_YI_HOME_1080P;

    table_offset = TABLE_HIGH_OFFSET_YI_HOME_1080P;
    stream_offset = STREAM_HIGH_OFFSET_YI_HOME_1080P;

    while (1) {
        static struct option long_options[] =
        {
            {"res",  required_argument, 0, 'r'},
            {"width",  required_argument, 0, '9'},
            {"height",  required_argument, 0, 'a'},
            {"model",  required_argument, 0, 'm'},
            {"table_offset",  required_argument, 0, '0'},
            {"table_record_size",  required_argument, 0, '1'},
            {"table_record_num",  required_argument, 0, '2'},
            {"buf_size",  required_argument, 0, '3'},
            {"stream_offset",  required_argument, 0, '4'},
            {"frame_counter_offset",  required_argument, 0, '5'},
            {"frame_offset_offset",  required_argument, 0, '6'},
            {"frame_length_offset",  required_argument, 0, '7'},
            {"frame_type_offset",  required_argument, 0, '8'},
            {"watermark",  no_argument, 0, 'w'},
            {"debug",  no_argument, 0, 'd'},
            {"help",  no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "r:9:a:m:0:1:2:3:4:5:6:7:8:wdh",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {
        case 'r':
            if (strcasecmp("low", optarg) == 0) {
                resolution = RESOLUTION_LOW;
            } else if (strcasecmp("high", optarg) == 0) {
                resolution = RESOLUTION_HIGH;
            }
            break;

        case 'm':
            if (strcasecmp("yi_home", optarg) == 0) {
                table_high_offset = TABLE_HIGH_OFFSET_YI_HOME;
                table_low_offset = TABLE_LOW_OFFSET_YI_HOME;
                table_record_size = TABLE_RECORD_SIZE_YI_HOME;
                table_record_num = TABLE_RECORD_NUM_YI_HOME;
                buf_size = BUF_SIZE_YI_HOME;
                stream_high_offset = STREAM_HIGH_OFFSET_YI_HOME;
                stream_low_offset = STREAM_LOW_OFFSET_YI_HOME;
                frame_counter_offset = FRAME_COUNTER_OFFSET_YI_HOME;
                frame_offset_offset = FRAME_OFFSET_OFFSET_YI_HOME;
                frame_length_offset = FRAME_LENGTH_OFFSET_YI_HOME;
                frame_type_offset = FRAME_TYPE_OFFSET_YI_HOME;
                w_low = W_LOW_YI_HOME;
                h_low = H_LOW_YI_HOME;
                w_high = W_HIGH_YI_HOME;
                h_high = H_HIGH_YI_HOME;
            } else if (strcasecmp("yi_home_1080p", optarg) == 0) {
                table_high_offset = TABLE_HIGH_OFFSET_YI_HOME_1080P;
                table_low_offset = TABLE_LOW_OFFSET_YI_HOME_1080P;
                table_record_size = TABLE_RECORD_SIZE_YI_HOME_1080P;
                table_record_num = TABLE_RECORD_NUM_YI_HOME_1080P;
                buf_size = BUF_SIZE_YI_HOME_1080P;
                stream_high_offset = STREAM_HIGH_OFFSET_YI_HOME_1080P;
                stream_low_offset = STREAM_LOW_OFFSET_YI_HOME_1080P;
                frame_counter_offset = FRAME_COUNTER_OFFSET_YI_HOME_1080P;
                frame_offset_offset = FRAME_OFFSET_OFFSET_YI_HOME_1080P;
                frame_length_offset = FRAME_LENGTH_OFFSET_YI_HOME_1080P;
                frame_type_offset = FRAME_TYPE_OFFSET_YI_HOME_1080P;
                w_low = W_LOW_YI_HOME_1080P;
                h_low = H_LOW_YI_HOME_1080P;
                w_high = W_HIGH_YI_HOME_1080P;
                h_high = H_HIGH_YI_HOME_1080P;
            } else if (strcasecmp("yi_dome_1080p", optarg) == 0) {
                table_high_offset = TABLE_HIGH_OFFSET_YI_DOME_1080P;
                table_low_offset = TABLE_LOW_OFFSET_YI_DOME_1080P;
                table_record_size = TABLE_RECORD_SIZE_YI_DOME_1080P;
                table_record_num = TABLE_RECORD_NUM_YI_DOME_1080P;
                buf_size = BUF_SIZE_YI_DOME_1080P;
                stream_high_offset = STREAM_HIGH_OFFSET_YI_DOME_1080P;
                stream_low_offset = STREAM_LOW_OFFSET_YI_DOME_1080P;
                frame_counter_offset = FRAME_COUNTER_OFFSET_YI_DOME_1080P;
                frame_offset_offset = FRAME_OFFSET_OFFSET_YI_DOME_1080P;
                frame_length_offset = FRAME_LENGTH_OFFSET_YI_DOME_1080P;
                frame_type_offset = FRAME_TYPE_OFFSET_YI_DOME_1080P;
                w_low = W_LOW_YI_DOME_1080P;
                h_low = H_LOW_YI_DOME_1080P;
                w_high = W_HIGH_YI_DOME_1080P;
                h_high = H_HIGH_YI_DOME_1080P;
            } else if (strcasecmp("yi_cloud_dome_1080p", optarg) == 0) {
                table_high_offset = TABLE_HIGH_OFFSET_YI_CLOUD_DOME_1080P;
                table_low_offset = TABLE_LOW_OFFSET_YI_CLOUD_DOME_1080P;
                table_record_size = TABLE_RECORD_SIZE_YI_CLOUD_DOME_1080P;
                table_record_num = TABLE_RECORD_NUM_YI_CLOUD_DOME_1080P;
                buf_size = BUF_SIZE_YI_CLOUD_DOME_1080P;
                stream_high_offset = STREAM_HIGH_OFFSET_YI_CLOUD_DOME_1080P;
                stream_low_offset = STREAM_LOW_OFFSET_YI_CLOUD_DOME_1080P;
                frame_counter_offset = FRAME_COUNTER_OFFSET_YI_CLOUD_DOME_1080P;
                frame_offset_offset = FRAME_OFFSET_OFFSET_YI_CLOUD_DOME_1080P;
                frame_length_offset = FRAME_LENGTH_OFFSET_YI_CLOUD_DOME_1080P;
                frame_type_offset = FRAME_TYPE_OFFSET_YI_CLOUD_DOME_1080P;
                w_low = W_LOW_YI_CLOUD_DOME_1080P;
                h_low = H_LOW_YI_CLOUD_DOME_1080P;
                w_high = W_HIGH_YI_CLOUD_DOME_1080P;
                h_high = H_HIGH_YI_CLOUD_DOME_1080P;
            } else if (strcasecmp("yi_dome", optarg) == 0) {
                table_high_offset = TABLE_HIGH_OFFSET_YI_DOME;
                table_low_offset = TABLE_LOW_OFFSET_YI_DOME;
                table_record_size = TABLE_RECORD_SIZE_YI_DOME;
                table_record_num = TABLE_RECORD_NUM_YI_DOME;
                buf_size = BUF_SIZE_YI_DOME;
                stream_high_offset = STREAM_HIGH_OFFSET_YI_DOME;
                stream_low_offset = STREAM_LOW_OFFSET_YI_DOME;
                frame_counter_offset = FRAME_COUNTER_OFFSET_YI_DOME;
                frame_offset_offset = FRAME_OFFSET_OFFSET_YI_DOME;
                frame_length_offset = FRAME_LENGTH_OFFSET_YI_DOME;
                frame_type_offset = FRAME_TYPE_OFFSET_YI_DOME;
                w_low = W_LOW_YI_DOME;
                h_low = H_LOW_YI_DOME;
                w_high = W_HIGH_YI_DOME;
                h_high = H_HIGH_YI_DOME;
            } else if (strcasecmp("yi_outdoor", optarg) == 0) {
                table_high_offset = TABLE_HIGH_OFFSET_YI_OUTDOOR;
                table_low_offset = TABLE_LOW_OFFSET_YI_OUTDOOR;
                table_record_size = TABLE_RECORD_SIZE_YI_OUTDOOR;
                table_record_num = TABLE_RECORD_NUM_YI_OUTDOOR;
                buf_size = BUF_SIZE_YI_OUTDOOR;
                stream_high_offset = STREAM_HIGH_OFFSET_YI_OUTDOOR;
                stream_low_offset = STREAM_LOW_OFFSET_YI_OUTDOOR;
                frame_counter_offset = FRAME_COUNTER_OFFSET_YI_OUTDOOR;
                frame_offset_offset = FRAME_OFFSET_OFFSET_YI_OUTDOOR;
                frame_length_offset = FRAME_LENGTH_OFFSET_YI_OUTDOOR;
                frame_type_offset = FRAME_TYPE_OFFSET_YI_OUTDOOR;
                w_low = W_LOW_YI_OUTDOOR;
                h_low = H_LOW_YI_OUTDOOR;
                w_high = W_HIGH_YI_OUTDOOR;
                h_high = H_HIGH_YI_OUTDOOR;
            }
            break;

        case 'w':
            watermark = 1;
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'a':
            errno = 0;    /* To distinguish success/failure after call */
            i_tmp = strtol(optarg, &endptr, 10);

            /* Check for various possible errors */
            if ((errno == ERANGE && (i_tmp == LONG_MAX || i_tmp == LONG_MIN)) || (errno != 0 && i_tmp == 0)) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            if (endptr == optarg) {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }

            if (c == '0') {
                table_high_offset = i_tmp;
                table_low_offset = i_tmp;
            } else if (c == '1') {
                table_record_size = i_tmp;
            } else if (c == '2') {
                table_record_num = i_tmp;
            } else if (c == '3') {
                buf_size = i_tmp;
            } else if (c == '4') {
                stream_high_offset = i_tmp;
                stream_low_offset = i_tmp;
            } else if (c == '5') {
                frame_counter_offset = i_tmp;
            } else if (c == '6') {
                frame_offset_offset = i_tmp;
            } else if (c == '7') {
                frame_length_offset = i_tmp;
            } else if (c == '8') {
                frame_type_offset = i_tmp;
            } else if (c == '9') {
                w_low = i_tmp;
                w_high = i_tmp;
            } else if (c == 'a') {
                h_low = i_tmp;
                h_high = i_tmp;
            }

            break;

        case 'd':
            fprintf(stderr, "Debug on\n");
            debug = 1;
            break;

        case 'h':
            print_usage(argv[0]);
            return -1;
            break;

        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            print_usage(argv[0]);
            return -1;
        }
    }

    if (debug) fprintf(stderr, "Starting program\n");

    // Check if snapshot is disabled
    if (access("/tmp/snapshot.disabled", F_OK ) == 0 ) {
        fprintf(stderr, "Snapshot is disabled\n");
        return 0;
    }

    // Check if snapshot is low res
    if (access("/tmp/snapshot.low", F_OK ) == 0 ) {
        fprintf(stderr, "Snapshot is low res\n");
        resolution = RESOLUTION_LOW;
    }

    // Check if the process is already running
    pid_t my_pid = getpid();
    if (proc_find(basename(argv[0]), my_pid) != -1) {
        fprintf(stderr, "Process is already running\n");
        return 0;
    }

    if (resolution == RESOLUTION_LOW) {
        table_offset = table_low_offset;
        stream_offset = stream_low_offset;
        width = w_low;
        height = h_low;
        fprintf(stderr, "Resolution low\n");
    } else if (resolution == RESOLUTION_HIGH) {
        table_offset = table_high_offset;
        stream_offset = stream_high_offset;
        width = w_high;
        height = h_high;
        fprintf(stderr, "Resolution high\n");
    }

    // Opening an existing file
    fFid = fopen(BUFFER_FILE, "r") ;
    if ( fFid == NULL ) {
        fprintf(stderr, "Could not open file %s\n", BUFFER_FILE) ;
        return -2;
    }

    // Map file to memory
    addr = (unsigned char*) mmap(NULL, buf_size, PROT_READ, MAP_SHARED, fileno(fFid), 0);
    if (addr == MAP_FAILED) {
        fprintf(stderr, "Error mapping file %s\n", BUFFER_FILE);
        fclose(fFid);
        return -3;
    }
    if (debug) fprintf(stderr, "%lld - mapping file %s, size %d, to %08x\n", current_timestamp(), BUFFER_FILE, buf_size, (unsigned int) addr);

    // Closing the file
    if (debug) fprintf(stderr, "%lld - closing the file %s\n", current_timestamp(), BUFFER_FILE) ;
    fclose(fFid) ;

    bufferh264 = NULL;
    bufferh264_size = 0;
    frame_type_sum = 0;

    // Find the record with the largest frame_counter
    current_frame = -1;
    frame_counter = -1;
    for (i = 0; i < table_record_num; i++) {
        // Get pointer to the record
        record_ptr = addr + table_offset + (i * table_record_size);
        // Get the frame counter
        frame_counter_tmp = (((int) *(record_ptr + frame_counter_offset + 1)) << 8) +
                    ((int) *(record_ptr + frame_counter_offset));
#if 0
        frame_type = (int) *(record_ptr + frame_type_offset);
        if (debug) fprintf(stderr, "%lld - frame_type %d\n", current_timestamp(), frame_type);
        if (frame_type == 7) {
            current_frame = i;
            break;
        }
#endif
        // Check if the is the largest frame_counter
        if (frame_counter_tmp > frame_counter) {
            frame_counter = frame_counter_tmp;
        } else {
            current_frame = i;
            break;
        }
    }
    if (debug) fprintf(stderr, "%lld - found latest frame: id %d, frame_counter %d\n", current_timestamp(), current_frame, frame_counter);

    start_time = time(NULL);

    // Wait for the next record to arrive and read the frame
    for (;;) {
        // Get pointer to the record
        record_ptr = addr + table_offset + (current_frame * table_record_size);
        if (debug) fprintf(stderr, "%lld - processing frame %d\n", current_timestamp(), current_frame);
        // Check if we are at the end of the table
        if (current_frame == table_record_num - 1) {
            next_record_ptr = addr + table_offset;
            if (debug) fprintf(stderr, "%lld - rewinding circular table\n", current_timestamp());
        } else {
            next_record_ptr = record_ptr + table_record_size;
        }
        // Get the frame counter of the next record
        next_frame_counter = ((int) *(next_record_ptr + frame_counter_offset + 1)) * 256 + ((int) *(next_record_ptr + frame_counter_offset));
        // Check if the frame counter is valid
        if (next_frame_counter >= frame_counter + 1) {
            // Get the frame type of the record
            frame_type = (int) *(record_ptr + frame_type_offset);
            // SPS, PPS or I-FRAME
            if ((frame_type == 7) || (frame_type == 8) || (frame_type == 5)) {
                frame_type_sum += frame_type;
                // Get the offset of the stream
                frame_offset = (((int) *(record_ptr + frame_offset_offset + 3)) << 24) +
                            (((int) *(record_ptr + frame_offset_offset + 2)) << 16) +
                            (((int) *(record_ptr + frame_offset_offset + 1)) << 8) +
                            ((int) *(record_ptr + frame_offset_offset));
                // Get the pointer to the frame address
                frame_ptr = addr + stream_offset + frame_offset;
                // Get the length of the frame
                frame_length = (((int) *(record_ptr + frame_length_offset + 3)) << 24) +
                            (((int) *(record_ptr + frame_length_offset + 2)) << 16) +
                            (((int) *(record_ptr + frame_length_offset + 1)) << 8) +
                            ((int) *(record_ptr + frame_length_offset));
                if (debug) fprintf(stderr, "%lld - writing frame: frame_offset %d, frame_ptr %08x, frame_length %d\n", current_timestamp(), frame_offset, (unsigned int) frame_ptr, frame_length);
                // Write the frame
                bufferh264 = (unsigned char *) realloc(bufferh264, bufferh264_size + frame_length);
                memcpy(&bufferh264[bufferh264_size], frame_ptr, frame_length);
                bufferh264_size += frame_length;

                if (frame_type_sum == 20) {
                    if (debug) fprintf(stderr, "%lld - frame found, exit loop\n", current_timestamp);
                    // We saved SPS, PPS and I-FRAME: exit loop and create JPEG image
                    break;
                }
            } else {
                if (bufferh264 != NULL) {
                    free(bufferh264);
                    bufferh264_size = 0;
                }
                frame_type_sum = 0;
            }

            // Check if we are at the end of the table
            if (current_frame == table_record_num - 1) {
                current_frame = 0;
            } else {
                current_frame++;
            }
        }

        current_time = time(NULL);
        if (current_time - start_time > TIMEOUT_INTERVAL) {
            fprintf(stderr, "Error, timeout expired and no frame found\n");
            return -4;
        }

        // Wait 10 milliseconds
        usleep(MILLIS_10);
    }

    if (bufferh264 == NULL) {
        fprintf(stderr, "Error, buffer is empty\n");
        return -5;
    }
    // Add FF_INPUT_BUFFER_PADDING_SIZE to make the size compatible with ffmpeg conversion
    bufferh264 = (unsigned char *) realloc(bufferh264, bufferh264_size + FF_INPUT_BUFFER_PADDING_SIZE);

    bufferyuv = (unsigned char *) malloc(width * height * 3 / 2);
    if (bufferyuv == NULL) {
        fprintf(stderr, "Unable to allocate memory\n");
        return -6;
    }

    if (debug) fprintf(stderr, "Decoding h264 frame\n");
    if(frame_decode(bufferyuv, bufferh264, bufferh264_size) < 0) {
        fprintf(stderr, "Error decoding h264 frame\n");
        return -7;
    }
    free(bufferh264);

    if (watermark) {
        if (debug) fprintf(stderr, "Adding watermark\n");
        if (add_watermark(bufferyuv, width, height) < 0) {
            fprintf(stderr, "Error adding watermark\n");
            return -8;
        }
    }

    if (debug) fprintf(stderr, "Encoding jpeg image\n");
    if(YUVtoJPG("stdout", bufferyuv, width, height, width, height) < 0) {
        fprintf(stderr, "Error encoding jpeg file\n");
        return -9;
    }

    free(bufferyuv);

    // Unmap file from memory
    if (munmap(addr, buf_size) == -1) {
        if (debug) fprintf(stderr, "Error munmapping file\n");
    } else {
        if (debug) fprintf(stderr, "Unmapping file %s, size %d, from %08x\n", BUFFER_FILE, buf_size, (unsigned int) addr);
    }

    return 0;
}
