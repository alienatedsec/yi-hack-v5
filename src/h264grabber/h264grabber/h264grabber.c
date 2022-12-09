/*
 * Copyright (c) 2022 roleo.
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
 * Dump h264 content from /tmp/view to fifo or stdout
 */

#define _GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>

// yi_home
#define TABLE_HIGH_OFFSET_YI_HOME 0x10
#define TABLE_LOW_OFFSET_YI_HOME 0x12E0
#define TABLE_AUDIO_OFFSET_YI_HOME 0x25B0
#define TABLE_RECORD_SIZE_YI_HOME 32
#define TABLE_RECORD_NUM_YI_HOME 150
#define BUF_SIZE_YI_HOME 648000
#define STREAM_HIGH_OFFSET_YI_HOME 0x4B40
#define STREAM_LOW_OFFSET_YI_HOME 0x68B40
#define STREAM_AUDIO_OFFSET_YI_HOME 0x8E340
#define FRAME_COUNTER_OFFSET_YI_HOME 18
#define FRAME_OFFSET_OFFSET_YI_HOME 4
#define FRAME_LENGTH_OFFSET_YI_HOME 8
#define FRAME_TS_OFFSET_YI_HOME 12

// yi_home_1080p
#define TABLE_HIGH_OFFSET_YI_HOME_1080P 0x10
#define TABLE_LOW_OFFSET_YI_HOME_1080P 0x25A0
#define TABLE_AUDIO_OFFSET_YI_HOME_1080P 0x4B30
#define TABLE_RECORD_SIZE_YI_HOME_1080P 32
#define TABLE_RECORD_NUM_YI_HOME_1080P 300
#define BUF_SIZE_YI_HOME_1080P 1586752
#define STREAM_HIGH_OFFSET_YI_HOME_1080P 0x9640
#define STREAM_LOW_OFFSET_YI_HOME_1080P 0x109640
#define STREAM_AUDIO_OFFSET_YI_HOME_1080P 0x163640
#define FRAME_COUNTER_OFFSET_YI_HOME_1080P 18
#define FRAME_OFFSET_OFFSET_YI_HOME_1080P 4
#define FRAME_LENGTH_OFFSET_YI_HOME_1080P 8
#define FRAME_TS_OFFSET_YI_HOME_1080P 12

// yi_dome_1080p
#define TABLE_HIGH_OFFSET_YI_DOME_1080P 0x10
#define TABLE_LOW_OFFSET_YI_DOME_1080P 0x25A0
#define TABLE_AUDIO_OFFSET_YI_DOME_1080P 0x4B30
#define TABLE_RECORD_SIZE_YI_DOME_1080P 32
#define TABLE_RECORD_NUM_YI_DOME_1080P 300
#define BUF_SIZE_YI_DOME_1080P 1586752
#define STREAM_HIGH_OFFSET_YI_DOME_1080P 0x9640
#define STREAM_LOW_OFFSET_YI_DOME_1080P 0x109640
#define STREAM_AUDIO_OFFSET_YI_DOME_1080P 0x163640
#define FRAME_COUNTER_OFFSET_YI_DOME_1080P 18
#define FRAME_OFFSET_OFFSET_YI_DOME_1080P 4
#define FRAME_LENGTH_OFFSET_YI_DOME_1080P 8
#define FRAME_TS_OFFSET_YI_DOME_1080P 12

// yi_cloud_dome_1080p
#define TABLE_HIGH_OFFSET_YI_CLOUD_DOME_1080P 0x10
#define TABLE_LOW_OFFSET_YI_CLOUD_DOME_1080P 0x25A0
#define TABLE_AUDIO_OFFSET_YI_CLOUD_DOME_1080P 0x4B30
#define TABLE_RECORD_SIZE_YI_CLOUD_DOME_1080P 32
#define TABLE_RECORD_NUM_YI_CLOUD_DOME_1080P 300
#define BUF_SIZE_YI_CLOUD_DOME_1080P 1586752
#define STREAM_HIGH_OFFSET_YI_CLOUD_DOME_1080P 0x9640
#define STREAM_LOW_OFFSET_YI_CLOUD_DOME_1080P 0x109640
#define STREAM_AUDIO_OFFSET_YI_CLOUD_DOME_1080P 0x163640
#define FRAME_COUNTER_OFFSET_YI_CLOUD_DOME_1080P 18
#define FRAME_OFFSET_OFFSET_YI_CLOUD_DOME_1080P 4
#define FRAME_LENGTH_OFFSET_YI_CLOUD_DOME_1080P 8
#define FRAME_TS_OFFSET_YI_CLOUD_DOME_1080P 12

// yi_dome - amended
#define TABLE_HIGH_OFFSET_YI_DOME 0x10
#define TABLE_LOW_OFFSET_YI_DOME 0x1920
#define TABLE_AUDIO_OFFSET_YI_DOME 0x3230
#define TABLE_RECORD_SIZE_YI_DOME 32
#define TABLE_RECORD_NUM_YI_DOME 200
#define BUF_SIZE_YI_DOME 654400
#define STREAM_HIGH_OFFSET_YI_DOME 0x6440
#define STREAM_LOW_OFFSET_YI_DOME 0x6A440
#define STREAM_AUDIO_OFFSET_YI_DOME 0x8FC40
#define FRAME_COUNTER_OFFSET_YI_DOME 18
#define FRAME_OFFSET_OFFSET_YI_DOME 4
#define FRAME_LENGTH_OFFSET_YI_DOME 8
#define FRAME_TS_OFFSET_YI_DOME 12

// yi_outdoor
#define TABLE_HIGH_OFFSET_YI_OUTDOOR 0x10
#define TABLE_LOW_OFFSET_YI_OUTDOOR 0x25A0
#define TABLE_AUDIO_OFFSET_YI_OUTDOOR 0x4B30
#define TABLE_RECORD_SIZE_YI_OUTDOOR 32
#define TABLE_RECORD_NUM_YI_OUTDOOR 300
#define BUF_SIZE_YI_OUTDOOR 1586752
#define STREAM_HIGH_OFFSET_YI_OUTDOOR 0x9640
#define STREAM_LOW_OFFSET_YI_OUTDOOR 0x109640
#define STREAM_AUDIO_OFFSET_YI_OUTDOOR 0x163640
#define FRAME_COUNTER_OFFSET_YI_OUTDOOR 18
#define FRAME_OFFSET_OFFSET_YI_OUTDOOR 4
#define FRAME_LENGTH_OFFSET_YI_OUTDOOR 8
#define FRAME_TS_OFFSET_YI_OUTDOOR 12

#define MILLIS_10 10000

#define RESOLUTION_NONE 0
#define RESOLUTION_LOW  360
#define RESOLUTION_HIGH 1080
#define RESOLUTION_AUDIO 1

#define BUFFER_FILE "/tmp/view"

#define FIFO_NAME_LOW "/tmp/h264_low_fifo"
#define FIFO_NAME_HIGH "/tmp/h264_high_fifo"
#define FIFO_NAME_AAC  "/tmp/aac_audio_fifo"

// Unused vars
unsigned char IDR[]               = {0x65, 0xB8};
unsigned char NAL_START[]         = {0x00, 0x00, 0x00, 0x01};
unsigned char IDR_START[]         = {0x00, 0x00, 0x00, 0x01, 0x65};
unsigned char PFR_START[]         = {0x00, 0x00, 0x00, 0x01, 0x61};
unsigned char SPS_START[]         = {0x00, 0x00, 0x00, 0x01, 0x67};
unsigned char PPS_START[]         = {0x00, 0x00, 0x00, 0x01, 0x68};

long long current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds

    return milliseconds;
}

void sigpipe_handler(int unused)
{
    // Do nothing
}

void print_usage(char *progname)
{
    fprintf(stderr, "\nUsage: %s [-r RES] [-d]\n\n", progname);
    fprintf(stderr, "\t-r RES, --resolution RES\n");
    fprintf(stderr, "\t\tset resolution: LOW, HIGH or AUDIO (default HIGH)\n");
    fprintf(stderr, "\t-m MODEL, --model MODEL\n");
    fprintf(stderr, "\t\tselect cam model: yi_home, yi_home_1080, yi_dome_1080p, yi_cloud_dome_1080p, yi_dome or yi_outdoor\n");
    fprintf(stderr, "\t--table_offset\n");
    fprintf(stderr, "\t\toffset of the table for the resolution selected\n");
    fprintf(stderr, "\t--table_record_size\n");
    fprintf(stderr, "\t\tsize of the record in the table\n");
    fprintf(stderr, "\t--table_record_num\n");
    fprintf(stderr, "\t\tnumber of record in the table\n");
    fprintf(stderr, "\t--buf_size\n");
    fprintf(stderr, "\t\tsize of the buffer file\n");
    fprintf(stderr, "\t--stream_offset\n");
    fprintf(stderr, "\t\toffset of the stream for the resolution selected\n");
    fprintf(stderr, "\t--frame_counter_offset\n");
    fprintf(stderr, "\t\toffset of the frame counter in the record\n");
    fprintf(stderr, "\t--frame_offset_offset\n");
    fprintf(stderr, "\t\toffset of the frame offset in the record\n");
    fprintf(stderr, "\t--frame_length_offset\n");
    fprintf(stderr, "\t\toffset of the frame lenght in the record\n");
    fprintf(stderr, "\t-f, --fifo\n");
    fprintf(stderr, "\t\tenable fifo output\n");
    fprintf(stderr, "\t-d, --debug\n");
    fprintf(stderr, "\t\tenable debug\n");
    fprintf(stderr, "\t-h, --help\n");
    fprintf(stderr, "\t\tprint this help\n");
}

int main(int argc, char **argv) {
    unsigned char *frame_ptr;
    unsigned int frame_offset;
    unsigned int frame_length;
    unsigned char *record_ptr, *next_record_ptr;

    FILE *fFid = NULL;
    FILE *fOut = NULL;

    int current_frame, frame_counter, frame_counter_tmp, next_frame_counter;
    int table_offset, stream_offset;
    unsigned int frame_ts = 0;
    unsigned int frame_ts_tmp, next_frame_ts, frame_ts_diff;
    int frame_counter_invalid = 0;

    int i, c, i_tmp;
    char *endptr;
    mode_t mode = 0755;

    int table_high_offset;
    int table_low_offset;
    int table_audio_offset;
    int table_record_size;
    int table_record_num;
    int buf_size;
    int stream_high_offset;
    int stream_low_offset;
    int stream_audio_offset;
    int frame_counter_offset;
    int frame_offset_offset;
    int frame_length_offset;
    int frame_ts_offset;

    unsigned char *addr;
    int resolution = RESOLUTION_HIGH;
    int debug = 0;
    int fifo = 0;

    // Settings default
    table_high_offset = TABLE_HIGH_OFFSET_YI_HOME_1080P;
    table_low_offset = TABLE_LOW_OFFSET_YI_HOME_1080P;
    table_audio_offset = TABLE_AUDIO_OFFSET_YI_HOME_1080P;
    table_record_size = TABLE_RECORD_SIZE_YI_HOME_1080P;
    table_record_num = TABLE_RECORD_NUM_YI_HOME_1080P;
    buf_size = BUF_SIZE_YI_HOME_1080P;
    stream_high_offset = STREAM_HIGH_OFFSET_YI_HOME_1080P;
    stream_low_offset = STREAM_LOW_OFFSET_YI_HOME_1080P;
    stream_audio_offset = STREAM_AUDIO_OFFSET_YI_HOME_1080P;
    frame_counter_offset = FRAME_COUNTER_OFFSET_YI_HOME_1080P;
    frame_offset_offset = FRAME_OFFSET_OFFSET_YI_HOME_1080P;
    frame_length_offset = FRAME_LENGTH_OFFSET_YI_HOME_1080P;
    frame_ts_offset = FRAME_TS_OFFSET_YI_HOME_1080P;

    table_offset = TABLE_HIGH_OFFSET_YI_HOME_1080P;
    stream_offset = STREAM_HIGH_OFFSET_YI_HOME_1080P;

    while (1) {
        static struct option long_options[] =
        {
            {"resolution",  required_argument, 0, 'r'},
            {"model",  required_argument, 0, 'm'},
            {"table_offset",  required_argument, 0, '0'},
            {"table_record_size",  required_argument, 0, '1'},
            {"table_record_num",  required_argument, 0, '2'},
            {"buf_size",  required_argument, 0, '3'},
            {"stream_offset",  required_argument, 0, '4'},
            {"frame_counter_offset",  required_argument, 0, '5'},
            {"frame_offset_offset",  required_argument, 0, '6'},
            {"frame_length_offset",  required_argument, 0, '7'},
            {"frame_ts_offset",  required_argument, 0, '8'},
            {"fifo",  no_argument, 0, 'f'},
            {"debug",  no_argument, 0, 'd'},
            {"help",  no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "r:m:0:1:2:3:4:5:6:7:8:fdh",
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
            } else if (strcasecmp("audio", optarg) == 0) {
                resolution = RESOLUTION_AUDIO;
            }
            break;

        case 'm':
            if (strcasecmp("yi_home", optarg) == 0) {
                table_high_offset = TABLE_HIGH_OFFSET_YI_HOME;
                table_low_offset = TABLE_LOW_OFFSET_YI_HOME;
                table_audio_offset = TABLE_AUDIO_OFFSET_YI_HOME;
                table_record_size = TABLE_RECORD_SIZE_YI_HOME;
                table_record_num = TABLE_RECORD_NUM_YI_HOME;
                buf_size = BUF_SIZE_YI_HOME;
                stream_high_offset = STREAM_HIGH_OFFSET_YI_HOME;
                stream_low_offset = STREAM_LOW_OFFSET_YI_HOME;
                stream_audio_offset = STREAM_AUDIO_OFFSET_YI_HOME;
                frame_counter_offset = FRAME_COUNTER_OFFSET_YI_HOME;
                frame_offset_offset = FRAME_OFFSET_OFFSET_YI_HOME;
                frame_length_offset = FRAME_LENGTH_OFFSET_YI_HOME;
                frame_ts_offset = FRAME_TS_OFFSET_YI_HOME;
            } else if (strcasecmp("yi_home_1080p", optarg) == 0) {
                table_high_offset = TABLE_HIGH_OFFSET_YI_HOME_1080P;
                table_low_offset = TABLE_LOW_OFFSET_YI_HOME_1080P;
                table_audio_offset = TABLE_AUDIO_OFFSET_YI_HOME_1080P;
                table_record_size = TABLE_RECORD_SIZE_YI_HOME_1080P;
                table_record_num = TABLE_RECORD_NUM_YI_HOME_1080P;
                buf_size = BUF_SIZE_YI_HOME_1080P;
                stream_high_offset = STREAM_HIGH_OFFSET_YI_HOME_1080P;
                stream_low_offset = STREAM_LOW_OFFSET_YI_HOME_1080P;
                stream_audio_offset = STREAM_AUDIO_OFFSET_YI_HOME_1080P;
                frame_counter_offset = FRAME_COUNTER_OFFSET_YI_HOME_1080P;
                frame_offset_offset = FRAME_OFFSET_OFFSET_YI_HOME_1080P;
                frame_length_offset = FRAME_LENGTH_OFFSET_YI_HOME_1080P;
                frame_ts_offset = FRAME_TS_OFFSET_YI_HOME_1080P;
            } else if (strcasecmp("yi_dome_1080p", optarg) == 0) {
                table_high_offset = TABLE_HIGH_OFFSET_YI_DOME_1080P;
                table_low_offset = TABLE_LOW_OFFSET_YI_DOME_1080P;
                table_audio_offset = TABLE_AUDIO_OFFSET_YI_DOME_1080P;
                table_record_size = TABLE_RECORD_SIZE_YI_DOME_1080P;
                table_record_num = TABLE_RECORD_NUM_YI_DOME_1080P;
                buf_size = BUF_SIZE_YI_DOME_1080P;
                stream_high_offset = STREAM_HIGH_OFFSET_YI_DOME_1080P;
                stream_low_offset = STREAM_LOW_OFFSET_YI_DOME_1080P;
                stream_audio_offset = STREAM_AUDIO_OFFSET_YI_DOME_1080P;
                frame_counter_offset = FRAME_COUNTER_OFFSET_YI_DOME_1080P;
                frame_offset_offset = FRAME_OFFSET_OFFSET_YI_DOME_1080P;
                frame_length_offset = FRAME_LENGTH_OFFSET_YI_DOME_1080P;
                frame_ts_offset = FRAME_TS_OFFSET_YI_DOME_1080P;
            } else if (strcasecmp("yi_cloud_dome_1080p", optarg) == 0) {
                table_high_offset = TABLE_HIGH_OFFSET_YI_CLOUD_DOME_1080P;
                table_low_offset = TABLE_LOW_OFFSET_YI_CLOUD_DOME_1080P;
                table_audio_offset = TABLE_AUDIO_OFFSET_YI_CLOUD_DOME_1080P;
                table_record_size = TABLE_RECORD_SIZE_YI_CLOUD_DOME_1080P;
                table_record_num = TABLE_RECORD_NUM_YI_CLOUD_DOME_1080P;
                buf_size = BUF_SIZE_YI_CLOUD_DOME_1080P;
                stream_high_offset = STREAM_HIGH_OFFSET_YI_CLOUD_DOME_1080P;
                stream_low_offset = STREAM_LOW_OFFSET_YI_CLOUD_DOME_1080P;
                stream_audio_offset = STREAM_AUDIO_OFFSET_YI_CLOUD_DOME_1080P;
                frame_counter_offset = FRAME_COUNTER_OFFSET_YI_CLOUD_DOME_1080P;
                frame_offset_offset = FRAME_OFFSET_OFFSET_YI_CLOUD_DOME_1080P;
                frame_length_offset = FRAME_LENGTH_OFFSET_YI_CLOUD_DOME_1080P;
                frame_ts_offset = FRAME_TS_OFFSET_YI_CLOUD_DOME_1080P;
            } else if (strcasecmp("yi_dome", optarg) == 0) {
                table_high_offset = TABLE_HIGH_OFFSET_YI_DOME;
                table_low_offset = TABLE_LOW_OFFSET_YI_DOME;
                table_audio_offset = TABLE_AUDIO_OFFSET_YI_DOME;
                table_record_size = TABLE_RECORD_SIZE_YI_DOME;
                table_record_num = TABLE_RECORD_NUM_YI_DOME;
                buf_size = BUF_SIZE_YI_DOME;
                stream_high_offset = STREAM_HIGH_OFFSET_YI_DOME;
                stream_low_offset = STREAM_LOW_OFFSET_YI_DOME;
                stream_audio_offset = STREAM_AUDIO_OFFSET_YI_DOME;
                frame_counter_offset = FRAME_COUNTER_OFFSET_YI_DOME;
                frame_offset_offset = FRAME_OFFSET_OFFSET_YI_DOME;
                frame_length_offset = FRAME_LENGTH_OFFSET_YI_DOME;
                frame_ts_offset = FRAME_TS_OFFSET_YI_DOME;
            } else if (strcasecmp("yi_outdoor", optarg) == 0) {
                table_high_offset = TABLE_HIGH_OFFSET_YI_OUTDOOR;
                table_low_offset = TABLE_LOW_OFFSET_YI_OUTDOOR;
                table_audio_offset = TABLE_AUDIO_OFFSET_YI_OUTDOOR;
                table_record_size = TABLE_RECORD_SIZE_YI_OUTDOOR;
                table_record_num = TABLE_RECORD_NUM_YI_OUTDOOR;
                buf_size = BUF_SIZE_YI_OUTDOOR;
                stream_high_offset = STREAM_HIGH_OFFSET_YI_OUTDOOR;
                stream_low_offset = STREAM_LOW_OFFSET_YI_OUTDOOR;
                stream_audio_offset = STREAM_AUDIO_OFFSET_YI_OUTDOOR;
                frame_counter_offset = FRAME_COUNTER_OFFSET_YI_OUTDOOR;
                frame_offset_offset = FRAME_OFFSET_OFFSET_YI_OUTDOOR;
                frame_length_offset = FRAME_LENGTH_OFFSET_YI_OUTDOOR;
                frame_ts_offset = FRAME_TS_OFFSET_YI_OUTDOOR;
            }
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
                table_audio_offset = i_tmp;
            } else if (c == '1') {
                table_record_size = i_tmp;
            } else if (c == '2') {
                table_record_num = i_tmp;
            } else if (c == '3') {
                buf_size = i_tmp;
            } else if (c == '4') {
                stream_high_offset = i_tmp;
                stream_low_offset = i_tmp;
                stream_audio_offset = i_tmp;
            } else if (c == '5') {
                frame_counter_offset = i_tmp;
            } else if (c == '6') {
                frame_offset_offset = i_tmp;
            } else if (c == '7') {
                frame_length_offset = i_tmp;
            }

            break;

        case 'f':
            fprintf (stderr, "Using fifo as output\n");
            fifo = 1;
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

    if (strcmp("h264grabber_l", basename(argv[0])) == 0) {
        resolution = RESOLUTION_LOW;
    } else if (strcmp("h264grabber_h", basename(argv[0])) == 0) {
        resolution = RESOLUTION_HIGH;
    } else if (strcmp("h264grabber_a", basename(argv[0])) == 0) {
        resolution = RESOLUTION_AUDIO;
    }

    if (resolution == RESOLUTION_LOW) {
        table_offset = table_low_offset;
        stream_offset = stream_low_offset;
        fprintf(stderr, "Resolution low\n");
    } else if (resolution == RESOLUTION_HIGH) {
        table_offset = table_high_offset;
        stream_offset = stream_high_offset;
        fprintf(stderr, "Resolution high\n");
    } else if (resolution == RESOLUTION_AUDIO) {
        table_offset = table_audio_offset;
        stream_offset = stream_audio_offset;
        fprintf(stderr, "AAC audio\n");
    }

    // Opening an existing file
    fFid = fopen(BUFFER_FILE, "r") ;
    if ( fFid == NULL ) {
        fprintf(stderr, "Could not open file %s\n", BUFFER_FILE) ;
        return -1;
    }

    // Map file to memory
    addr = (unsigned char*) mmap(NULL, buf_size, PROT_READ, MAP_SHARED, fileno(fFid), 0);
    if (addr == MAP_FAILED) {
        fprintf(stderr, "Error mapping file %s\n", BUFFER_FILE);
        fclose(fFid);
        return -2;
    }
    if (debug) fprintf(stderr, "%lld - mapping file %s, size %d, to %08x\n", current_timestamp(), BUFFER_FILE, buf_size, (unsigned int) addr);

    // Closing the file
    if (debug) fprintf(stderr, "%lld - closing the file %s\n", current_timestamp(), BUFFER_FILE) ;
    fclose(fFid) ;

    if (fifo == 0) {
        char stdoutbuf[262144];

        if (setvbuf(stdout, stdoutbuf, _IOFBF, sizeof(stdoutbuf)) != 0) {
            fprintf(stderr, "Error setting stdout buffer\n");
        }
        fOut = stdout;
    } else {
        sigaction(SIGPIPE, &(struct sigaction){{sigpipe_handler}}, NULL);

        if (resolution == RESOLUTION_LOW) {
            unlink(FIFO_NAME_LOW);
            if (mkfifo(FIFO_NAME_LOW, mode) < 0) {
                fprintf(stderr, "mkfifo failed for file %s\n", FIFO_NAME_LOW);
                return -1;
            }
            fOut = fopen(FIFO_NAME_LOW, "w");
            if (fOut == NULL) {
                fprintf(stderr, "Error opening fifo %s\n", FIFO_NAME_LOW);
                return -1;
            }
        } else if (resolution == RESOLUTION_HIGH) {
            unlink(FIFO_NAME_HIGH);
            if (mkfifo(FIFO_NAME_HIGH, mode) < 0) {
                fprintf(stderr, "mkfifo failed for file %s\n", FIFO_NAME_HIGH);
                return -1;
            }
            fOut = fopen(FIFO_NAME_HIGH, "w");
            if (fOut == NULL) {
                fprintf(stderr, "Error opening fifo %s\n", FIFO_NAME_HIGH);
                return -1;
            }
        } else if (resolution == RESOLUTION_AUDIO) {
            unlink(FIFO_NAME_AAC);
            if (mkfifo(FIFO_NAME_AAC, mode) < 0) {
                fprintf(stderr, "mkfifo failed for file %s\n", FIFO_NAME_AAC);
                return -1;
            }
            fOut = fopen(FIFO_NAME_AAC, "w");
            if (fOut == NULL) {
                fprintf(stderr, "Error opening fifo %s\n", FIFO_NAME_AAC);
                return -1;
            }
        }
    }

    if ((resolution == RESOLUTION_LOW) || (resolution == RESOLUTION_HIGH)) {
        for(;;) {
            // Find the record with the largest frame_counter
            current_frame = -1;
            frame_counter = -1;
            for (i = 0; i < table_record_num; i++) {
                // Get pointer to the record
                record_ptr = addr + table_offset + (i * table_record_size);
                // Get the frame counter
                frame_counter_tmp = (((int) *(record_ptr + frame_counter_offset + 1)) << 8) +
                            ((int) *(record_ptr + frame_counter_offset));
                // Check if it is the largest frame_counter
                if (frame_counter_tmp > frame_counter) {
                    frame_counter = frame_counter_tmp;
                } else {
                    current_frame = i;
                    break;
                }
            }
            if (debug) fprintf(stderr, "%lld - found latest frame: id %d, frame_counter %d\n", current_timestamp(), current_frame, frame_counter);

            // Wait 10 milliseconds
            usleep(MILLIS_10);

            // Wait for the next record to arrive and read the frame
            for (;;) {
                // Get pointer to the record
                record_ptr = addr + table_offset + (current_frame * table_record_size);
                if (debug) fprintf(stderr, "%lld - processing frame %d, frame_counter %d\n", current_timestamp(), current_frame, frame_counter);
                // Check if we are at the end of the table
                if (current_frame == table_record_num - 1) {
                    next_record_ptr = addr + table_offset;
                    if (debug) fprintf(stderr, "%lld - rewinding circular table\n", current_timestamp());
                } else {
                    next_record_ptr = record_ptr + table_record_size;
                }
                // Get the frame counter of the next record
                next_frame_counter = ((int) *(next_record_ptr + frame_counter_offset + 1)) * 256 + ((int) *(next_record_ptr + frame_counter_offset));
                if (debug) fprintf(stderr, "%lld - current frame counter is %d, next frame counter is %d\n", current_timestamp(), frame_counter, next_frame_counter);
                // Check if the frame counter is valid
                if (((next_frame_counter - frame_counter > 0) && (next_frame_counter - frame_counter <= 10)) ||
                            ((next_frame_counter + 65536 - frame_counter > 0) && (next_frame_counter + 65536 - frame_counter <= 10))) {
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
                    if (debug) fprintf(stderr, "%lld - writing frame: frame_counter %d, frame_offset %d, frame_ptr %08x, frame_length %d\n", current_timestamp(), frame_counter, frame_offset, (unsigned int) frame_ptr, frame_length);
                    // Write the frame
                    fwrite(frame_ptr, 1, frame_length, fOut);
                    fflush(fOut);

                    // Check if we are at the end of the table
                    if (current_frame == table_record_num - 1) {
                        current_frame = 0;
                    } else {
                        current_frame++;
                    }
                    frame_counter = next_frame_counter;
                    frame_counter_invalid = 0;
                } else {
                    frame_counter_invalid++;
                    if (frame_counter_invalid < 10) {
                        if (debug) fprintf(stderr, "%lld - frame counter invalid %d\n", current_timestamp(), frame_counter_invalid);
                    } else {
                        if (debug) fprintf(stderr, "%lld - frame counter invalid %d, sync lost\n", current_timestamp(), frame_counter_invalid);
                        break;
                    }
                }

                // Wait 10 milliseconds
                usleep(MILLIS_10);
            }
        }
    } else if (resolution == RESOLUTION_AUDIO) {
        for(;;) {
            // Find the record with the largest timestamp
            current_frame = -1;
            frame_counter = -1;
            for (i = 0; i < table_record_num; i++) {
                // Get pointer to the record
                record_ptr = addr + table_offset + (i * table_record_size);
                // Get the frame timestamp
                frame_ts_tmp = (((int) *(record_ptr + frame_ts_offset + 3)) << 24) +
                            (((int) *(record_ptr + frame_ts_offset + 2)) << 16) +
                            (((int) *(record_ptr + frame_ts_offset + 1)) << 8) +
                            ((int) *(record_ptr + frame_ts_offset));
                // Check if it is the largest timestamp
                if (frame_ts_tmp > frame_ts) {
                    frame_ts = frame_ts_tmp;
                } else {
                    current_frame = i;
                    break;
                }
            }
            if (debug) fprintf(stderr, "%lld - found latest frame: id %d, frame_ts %u\n", current_timestamp(), current_frame, frame_ts);

            // Wait 10 milliseconds
            usleep(MILLIS_10);

            // Wait for the next record to arrive and read the frame
            for (;;) {
                // Get pointer to the record
                record_ptr = addr + table_offset + (current_frame * table_record_size);
                if (debug) fprintf(stderr, "%lld - processing frame %d, frame_ts %u\n", current_timestamp(), current_frame, frame_ts);
                // Check if we are at the end of the table
                if (current_frame == table_record_num - 1) {
                    next_record_ptr = addr + table_offset;
                    if (debug) fprintf(stderr, "%lld - rewinding circular table\n", current_timestamp());
                } else {
                    next_record_ptr = record_ptr + table_record_size;
                }
                // Get the frame timestamp of the next record
                next_frame_ts = (((int) *(next_record_ptr + frame_ts_offset + 3)) << 24) +
                            (((int) *(next_record_ptr + frame_ts_offset + 2)) << 16) +
                            (((int) *(next_record_ptr + frame_ts_offset + 1)) << 8) +
                            ((int) *(next_record_ptr + frame_ts_offset));
                if (debug) fprintf(stderr, "%lld - current frame timestamp is %u, next frame timestamp is %u\n", current_timestamp(), frame_ts, next_frame_ts);
                // Check if the frame timestamp is valid
                if (next_frame_ts >= frame_ts)
                    frame_ts_diff = next_frame_ts - frame_ts;
                else
                    frame_ts_diff = next_frame_ts + (0xffffffff - frame_ts + 1);
                if ((frame_ts_diff >= 0) && (frame_ts_diff <= 10 * 128)) {
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
                    if (debug) fprintf(stderr, "%lld - writing frame: frame_ts %u, frame_offset %d, frame_ptr %08x, frame_length %d\n", current_timestamp(), frame_ts, frame_offset, (unsigned int) frame_ptr, frame_length);
                    // Write the frame
                    fwrite(frame_ptr, 1, frame_length, fOut);
                    fflush(fOut);

                    // Check if we are at the end of the table
                    if (current_frame == table_record_num - 1) {
                        current_frame = 0;
                    } else {
                        current_frame++;
                    }
                    frame_ts = next_frame_ts;
                    frame_counter_invalid = 0;
                } else {
                    frame_counter_invalid++;
                    if (frame_counter_invalid < 10) {
                        if (debug) fprintf(stderr, "%lld - frame counter invalid %d\n", current_timestamp(), frame_counter_invalid);
                    } else {
                        if (debug) fprintf(stderr, "%lld - frame counter invalid %d, sync lost\n", current_timestamp(), frame_counter_invalid);
                        break;
                    }
                }

                // Wait 10 milliseconds
                usleep(MILLIS_10);
            }
        }
    }

    // Unreacheable path

    if (fifo == 1) {
        if (resolution == RESOLUTION_LOW) {
            fclose(fOut);
            unlink(FIFO_NAME_LOW);
        } else if (resolution == RESOLUTION_HIGH) {
            fclose(fOut);
            unlink(FIFO_NAME_HIGH);
        } else if (resolution == RESOLUTION_AUDIO) {
            fclose(fOut);
            unlink(FIFO_NAME_AAC);
        }
    }

    // Unmap file from memory
    if (munmap(addr, buf_size) == -1) {
        if (debug) fprintf(stderr, "Error munmapping file\n");
    } else {
        if (debug) fprintf(stderr, "Unmapping file %s, size %d, from %08x\n", BUFFER_FILE, buf_size, (unsigned int) addr);
    }

    return 0;
}
