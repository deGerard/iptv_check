/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "rtp.h"
#include "log_print.h"

struct rtp_hdr {
    unsigned int version:2;  /* protocol version */
    unsigned int p:1;        /* padding flag */
    unsigned int x:1;        /* header extension flag */
    unsigned int cc:4;       /* CSRC count */
    unsigned int m:1;        /* marker bit */
    unsigned int pt:7;       /* payload type */
    uint16_t seq;                     /* sequence number */
    uint32_t ts;                      /* timestamp */
    uint32_t ssrc;                    /* synchronization source */
    uint32_t csrc[1];                 /* optional CSRC list */
};

static uint16_t previous_seq=0;
static time_t prev_time;
static uint32_t data_count = 0;
static uint32_t first_time = 1;

extern int runasdaemon;


int rtp_in(uint8_t *data, int32_t len)
{
    struct rtp_hdr *hdr = (struct rtp_hdr*)data;
    int16_t diff;
    uint16_t seq = ntohs(hdr->seq);
    struct tm *tm;
    time_t now;
    const char loopchar[4] = "|/-\\";
    static int loopcharcntr = 0;

    data_count+=len;
    diff = seq - previous_seq;

    time(&now);
    if (first_time) {
        first_time = 0;
        tm = gmtime(&now);
        log_printf ("[%02d-%02d-%04d %02d:%02d:%02d] ", tm->tm_mday, tm->tm_mon+1, tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
        log_printf ("start @ sequence: 0x%04x, timestamp: %u\n", seq, ntohl(hdr->ts));
    }
    else if (diff != 1) {
        tm = gmtime(&now);
        log_printf ("[%02d-%02d-%04d %02d:%02d:%02d] ", tm->tm_mday, tm->tm_mon+1, tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
        log_printf ("lost packets between sequence 0x%04x and 0x%04x, lost %d frames @ timestamp: %u\n", previous_seq, seq, diff, ntohl(hdr->ts));
    }
    previous_seq = seq;

    if (!runasdaemon) {
        /* Not go to measure the bitrate when we are in daemon mode */
        if (prev_time != now) {
            prev_time = now;
            data_count = data_count * 8;    // Calculate bytes per second to bits per second
            printf ("%c receive rate: ", loopchar[loopcharcntr]);
            loopcharcntr = (loopcharcntr + 1) & 3;

            if (data_count < 1024) {
                printf ("%u bps", data_count);
            }
            else if (data_count < (1024 * 1024)) {
                printf ("%.2f kbps", (float)data_count / 1024);
            }
            else if (data_count < (1024 * 1024 * 1024)) {
                printf ("%.2f Mbps", (float)data_count / (1024 * 1024));
            }

            printf ("        \r");
            fflush(stdout);
            data_count = 0;
        }
    }

    return 0;
}
