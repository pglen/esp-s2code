
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


int cntS = 0;
int cntR = 0;

char buffer[128] = "";
char payload[128] = "Hello ";
char buff2[128] = "";
char buff3[256] = "";

#include "../../packets.c"

void task_tx(void *p) {

    vTaskDelay(pdMS_TO_TICKS(300));

    for(;;) {
        //uint16_t rrr = esp_random() & 0xffff;
        //int slen = snprintf(buffer, sizeof(buffer),
        //        "At: %ld, rrr=%u ser %d Hello World.", tttt, rrr, cntS);
        //printf("%s ... ", buffer);
        //fflush(stdout);

        //lora_set_tx_power(cntS);

        int32_t tttt = (int32_t)(esp_timer_get_time() / 1000);

        snprintf(buff3, sizeof(buff3), "'%s' %3d\n", payload, cntS);

        Lora_Packet pack;
        pack.sid = 0;
        pack.tid = 0;

        int slen = assemble_packet(&pack, buff3, buffer, sizeof(buffer));
        //hexdump(buffer, slen, buff3, sizeof(buff3));
        lora_send_packet((uint8_t *)buffer, slen);
        int32_t tttt2 = (int32_t)(esp_timer_get_time() / 1000);
        sprintf(buff2, "Sent: cnt=%d len=%d send time: %ldms ",
                                            cntS, slen, tttt2-tttt);
        out_str(buff2, buff3);
        cntS++;
        toggle_led(1);
        vTaskDelay(pdMS_TO_TICKS(500));
        }
}

void app_main() {

    configure_led();

    led_strip_set_pixel(led_strip, 0, 0, 0, 0);

    lora_hw_init(); // Initialize the LoRa module (pins configured in menuconfig)
    init_lora_common();     // Send / recv common config

    // Transmitter, override power to MAX
    lora_set_tx_power(15);
    lora_set_boost(1);

    xTaskCreate(&task_tx, "task_tx", 2048, NULL, 5, NULL);
    //xTaskCreate(&task_rx, "task_rx", 2048, NULL, 5, NULL);
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
