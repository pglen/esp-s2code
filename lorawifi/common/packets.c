
/* =====[ loratest.sess ]========================================================

   File Name:       packets.c

   Description:     Functions for packets.c

   Revisions:

      REV   DATE                BY              DESCRIPTION
      ----  -----------         ----------      --------------------------
      0.00  Mon 12.Jan.2026     Peter Glen      Initial version.

   ======================================================================= */

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_timer.h>
#include <esp_https_server.h>
#include <esp_http_server.h>
#include <esp_tls.h>

#include "protocol.h"
#include "utils.h"
#include "comline.h"
#include "packets.h"
#include "leds.h"
#include "lora.h"

static const char *TAG = "packets";

// Shuffle a string to 16 bit unique ID

int16_t lora_chksum(const char *str, int len)
{
    //printf("len %d str '%s'\n", len, str);
    uint16_t ret = 0;
    for(int aa = 0; aa < len; aa++)
        {
        uint16_t nn = (uint16_t)str[aa];
        uint16_t qq = nn << 7 | nn;
        ret += qq + 10000;
        ret ^= 0x5aa5;
        }
    //printf("sum ret %d\n", ret);
    return(ret);
}

// Store it in little endian (intel) order
// Note: ESP32 is big andian ...
// ... This is why we are not using structures

int     assemble_packet(const char *pay, uint16_t trench, char *outstr, int maxlen)
{
    int prog = 0;
    uint16_t ccc = lora_chksum(pay, strlen(pay));
    //printf("ccc %d (0x%x)\n" , ccc, ccc);

    outstr[prog] = ccc & 0xff; prog++;
    outstr[prog] = ccc >> 8; prog++;

    int lenoffs = prog;
    outstr[prog] = 0 & 0xff; prog++;

    outstr[prog] = trench & 0xff; prog++;
    outstr[prog] = trench >> 8; prog++;

    int slen = snprintf(outstr + prog, maxlen - prog, "%s", pay);
    outstr[lenoffs] = slen & 0xff;
    return(prog += slen);
}

int     disass_packet(const char *instr, int inlen, uint16_t *hash, uint16_t *trench)
{


    return 0;
}

// Verify is valid packet

int     check_packet(const char *str, int len)

{
    uint16_t nsum = lora_chksum(str + 5, *(str+2) );
    uint16_t org = *(str) + (*(str+1) << 8);
    //printf("nsum %d org %d\n", nsum, org);
    return nsum == org;
}

void    send_payload(const char * buff)

{
    char    buffer[256];   buffer[0] = '\0';

    rr = 100, gg = 50, bb = 0;
    blink_cnt = 1;

    printf("Sending: %s\n", buff);
    int slen = assemble_packet(buff, atoi(gl_curr_tr), buffer, sizeof(buffer));
    int32_t tttt = (int32_t)(esp_timer_get_time() / 1000);
    TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
    lora_send_packet((uint8_t *)buffer, slen);
    //int ret = lora_read_freq_err();
    GIVE_SEMA(sSemaphore);
    int32_t tttt2 = (int32_t)(esp_timer_get_time() / 1000);
    printf("Send time: %ld ms\n", tttt2-tttt);
    blink_cnt = 2;
    //print_freq_deviation(ret);
}

//# define LINUX_TEST
#define INCLUDE_DUMP

int     isalnum2(char chh)
{
    if(chh >= '0' && chh <= '9')
        return TRUE;
    if(chh >= ':' && chh <= '@')
        return TRUE;
    if(chh >= 'a' && chh <= 'z')
        return TRUE;
    if(chh >= '[' && chh <= '`')
        return TRUE;
    if(chh >= 'A' && chh <= 'Z')
        return TRUE;
    if(chh >= ' ' && chh <= '/')
        return TRUE;
    if(chh >= '{' && chh <= '~')
        return TRUE;
    return FALSE;
}

void    sdump(const char *str, int len, char *out, int maxlen)
{
    int aa = 0, was = 0, prog = 0;
    for(aa = 0; aa < len; aa++)
        {
        if(prog >= maxlen)
            {
            out[maxlen] = '\0';
            printf("sdump overflow at: %d\n", prog);
            return;
            }
        char bb = str[aa] & 0xff;
        if(isalnum2(bb))
            {
            prog += sprintf(out+prog, "%c", bb);
            was = 1;
            }
        else
            {
            if (was)
                prog += sprintf(out+prog, " ");
            prog += sprintf(out+prog, "%02x ", bb);
            was = 0;
            }
        //if (aa % 16 == 15)
        //    {
        //    prog += sprintf(out+prog, "\n");
        //    }
        }
    if (aa % 16 != 15)
        prog += sprintf(out+prog, "\n");
}

# ifdef INCLUDE_DUMP

void    hexdump(const char *str, int len, char *out, int maxlen)
{
    int aa = 0, sss = 0, prog = 0;
    for(aa = 0; aa < len; aa++)
        {
        if(prog >= maxlen)
            {
            out[maxlen] = '\0';
            printf("hexdump overflow at: %d\n", prog);
            return;
            }
        //if (aa % 16 == 0)
        //    prog += sprintf(out+prog, "0x%-3.2x- ", aa);
        prog += sprintf(out+prog, "%02x ", str[aa] & 0xff);
        if (aa % 16 == 15)
            {
            prog += sprintf(out+prog, " -  ");
            for(int bb = sss; bb < sss + 16; bb++)
                {
                if(prog >= maxlen)
                    {
                    out[maxlen] = '\0';
                    printf("hexdump overflow at: %d\n", prog);
                    return;
                    }
                if(isalnum2(str[bb]))
                    prog += sprintf(out+prog, "%c", str[bb]);
                else
                    prog += sprintf(out+prog, ".");
                }
            prog += sprintf(out+prog, "\n");
            sss = aa + 1;
            }
        }
    if (sss != aa )
        {
        for(int cc = aa; cc < aa + 16 - len % 16; cc++)
            {
            if(prog >= maxlen)
                {
                out[maxlen] = '\0';
                printf("hexdump overflow at: %d\n", prog);
                return;
                }

            prog += sprintf(out+prog, "   ");
            }
        prog += sprintf(out+prog, " -  ");
        for(int bb = sss; bb < sss + 16; bb++)
            {
            if(prog >= maxlen)
            {
            out[maxlen] = '\0';
            printf("hexdump overflow at: %d\n", prog);
            return;
            }

            if (isalnum2(str[bb]))
                prog += sprintf(out+prog, "%c", str[bb]);
            else
                prog += sprintf(out+prog, ".");
            }
        //sprintf(out, "sss %d aa, %d", sss, aa);
        }
    prog += sprintf(out+prog, "\n");
    *(out+prog) = '\0';
}

# endif

// Common init

void    init_lora_common()

{
    //lora_enable_crc(); // Enable CRC check
    lora_set_frequency(atofx(gl_txfreq));
    lora_set_tx_power(atofx(gl_txpower));
    lora_set_boost(1);
    lora_set_spreading_factor(atofx(gl_spread));
    lora_set_bandwidth(atofx(gl_bwidth));
}

#ifdef LINUX_TEST

char buff[64] = "";

#if 0
int     main(int argv, char* argc[])

{
    srandom(time(NULL));
    int len = assemble_packet("hello 0123456789\n", 1234, buff, sizeof(buff));

    # ifdef  INCLUDE_DUMP
    hexdump(buff, len);
    # endif

    int ret = check_packet(buff, len);
    printf("Check: %d\n", ret);

    //char *strs;
    //printf("sum: %x\n", chksum(strs, strlen(strs)) & 0xffff);
    //strs = "12345678";
    //printf("sum: %x\n", chksum(strs, strlen(strs)) & 0xffff);
    //strs = "12345679";
    //printf("sum: %x\n", chksum(strs, strlen(strs)) & 0xffff);
    //strs = "22345679";
    //printf("sum: %x\n", chksum(strs, strlen(strs)) & 0xffff);

    return 0;
}

#endif

#endif
