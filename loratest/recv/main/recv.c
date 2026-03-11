
/* =====[ qcan2.sess ]========================================================

   File Name:       recv.c

   Description:     Functions for recv.c

   Revisions:

      REV   DATE                BY              DESCRIPTION
      ----  -----------         ----------      --------------------------
      0.00  Tue 13.Jan.2026     Peter Glen      Initial version.

   ======================================================================= */

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lora.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_timer.h"
#include "esp_console.h"

char payload[128] = "Hello ";

#include "../../packets.c"

int cntS = 0;
int cntR = 0;

char buffer[128] = "";
char buff2[128]  = "";
char buff3[128]  = "";

void task_rx(void *p) {

    //printf("Started recv\n");

    int reclen = 0;

    for(;;) {
        lora_receive();
        reclen = lora_receive_packet((uint8_t*)buffer, sizeof(buffer));
        if (reclen == 0)
            {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
            }
        int OK = check_packet(buffer, reclen);
        toggle_led(OK);
        if(verbose)
            {
            int slen = snprintf(buff2, sizeof(buff2) - 1,
                    "Recvd: %d len: %d rssi: %d cnt: %d check: %d\n",
                        cntR, reclen, lora_packet_rssi(), cntR, OK);
            buff2[slen] = '\0';
            }
        else
            {
            buff2[0] = '\0';
            }
        cntR++;
        //fflush(stdout);
        //if(rec >= sizeof(buff)-1) { rec = sizeof(buffer)-1; }
        //  buffer[rec] = 0;
        //hexdump(buffer, reclen, buff3, sizeof(buff3)-1);
        sdump(buffer, reclen, buff3, sizeof(buff3)-1);
        //printf("buff2 '%s'\n", buff2);
        //printf("buff3 '%s'\n", buff3);
        out_str(buff2, buff3);
        //vTaskDelay(pdMS_TO_TICKS(100));
        }
}

void app_main() {

    configure_led();

    led_strip_set_pixel(led_strip, 0, 0, 0, 0);

    lora_hw_init();         // Initialize LoRa
    init_lora_common();     // Send / recv common config

    xTaskCreate(&task_rx, "task_rx", 2048, NULL, 5, NULL);
    //vTaskDelay(pdMS_TO_TICKS(100));
    printf("\033[1;24r");
    printf(VT_CLEAR_SCREEN);
    printf("\033[13;24r");
    printf(VT_SETWIN_CLEAR);
    printf("\033[24;1H");
    vTaskDelay(pdMS_TO_TICKS(100));
    start_console("LORA>");
    //printf("\n");

    while(1)
        vTaskDelay(pdMS_TO_TICKS(10));
}

// EOF
