
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
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <esp_https_server.h>
#include <esp_http_server.h>
#include <esp_tls.h>

#include "httpd.h"
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
    uint16_t hhh = lora_chksum(pay, strlen(pay));
    //printf("hhh %d (0x%x)\n" , hhh, hhh);

    outstr[prog] = hhh & 0xff; prog++;
    outstr[prog] = hhh >> 8; prog++;

    int lenoffs = prog;
    outstr[prog] = 0 & 0xff; prog++;

    outstr[prog] = trench & 0xff; prog++;
    outstr[prog] = trench >> 8; prog++;

    int slen = snprintf(outstr + prog, maxlen - prog, "%s", pay);
    outstr[lenoffs] = slen & 0xff;
    return(prog += slen);
}

int     disass_packet(const char *instr, uint16_t *hash, uint16_t *trench, const char **out)
{
    uint16_t hhh, ttt;  int prog = 0;
    hhh = instr[prog] & 0xff; prog++;
    hhh |= instr[prog] << 8; prog++;
    *hash = hhh;
    int lenx = instr[prog] & 0xff; prog++;
    ttt = instr[prog] & 0xff; prog++;
    ttt |= instr[prog] << 8; prog++;
    *trench = ttt;
    *out =  &instr[prog];
    return lenx;
}

// Verify if it is a valid packet

int     check_packet(const char *str, int len)

{
    uint16_t nsum = lora_chksum(str + 5, *(str+2) );
    uint16_t org = *(str) + (*(str+1) << 8);
    //printf("nsum %d org %d\n", nsum, org);
    return nsum == org;
}

void    send_payload(const char * buff, uint16_t trench)

{
    gl_recala = 0;      // Clear rec LED
    char    buffer[256];   buffer[0] = '\0';
    blink_led(1, 100, 50, 0);

    printf("Sending: trench=%d str: '%s'\n", trench, buff);
    int slen = assemble_packet(buff, trench, buffer, sizeof(buffer));
    int32_t tttt = (int32_t)(esp_timer_get_time() / 1000);
    TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
    lora_send_packet((uint8_t *)buffer, slen);
    //int ret = lora_read_freq_err();
    GIVE_SEMA(sSemaphore);
    int32_t tttt2 = (int32_t)(esp_timer_get_time() / 1000);
    printf("Send time: %ld ms\n", tttt2-tttt);
    //blink_cnt = 2;
    blink_led(2, 100, 50, 0);
    //print_freq_deviation(ret);
}

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
