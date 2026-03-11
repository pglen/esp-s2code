
/* =====[ access point template project ]=================================

   File Name:       common.c

   Description:

   Revisions:

      REV       DATE               BY          DESCRIPTION
      ----  -----------         ----------      -------------------------
      0.00  Tue 10.Mar.2026     Peter Glen      Initial version.

   ======================================================================= */

// ESP32 19 pins 8 in (4x4) + 8 out

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"

#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"

#include "nvs_flash.h"

#include "esp_wifi_types.h"
#include "esp_console.h"
#include "esp_task_wdt.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_crc.h"

//#include "esp32/ulp.h"
#include "esp_sleep.h"
#include "esp32/rom/crc.h"

#include <esp_http_server.h>

#include "common.h"
#include "protocol.h"
#include "sere.h"
#include "wifi.h"
#include "input.h"

static  int     idle_cnt = 0;
static  int     new_cnt = 0;
static  int     usage_inited = 0;

static void    idle_task(void *parm)

{
    while(1==1)
        {
        //printf("#");
        int64_t now = esp_timer_get_time();
        vTaskDelay(0 / portTICK_RATE_MS);
        int64_t now2 = esp_timer_get_time();
        idle_cnt += (now2 - now) / 1000;
        }
}

// Exposed:
int     gl_enable_measure = 0;

static void    idle_task2(void *parm)

{
    int cnt = 0;

    while(1==1)
        {
        new_cnt =  idle_cnt;        // Save the count for printing it ...
        idle_cnt = 0;               // Reset

        cnt++;
        if(gl_enable_measure)
            {
            //if(cnt % 2 == 0)
                {
                printf(" cpu=%d ", new_cnt); fflush(stdout);
                }
            // Warn if too low
            if(new_cnt < 200 && new_cnt > 0)
                {
                //ESP_LOGE(TAG, "cpu=%d ", new_cnt);
                }
            }
        vTaskDelay(1000 / portTICK_RATE_MS);
        }
}

void    init_cpu_measure()

{
    usage_inited = true;
    xTaskCreate(idle_task, "idle_task", 1024 * 2, NULL,  0, NULL);
    xTaskCreate(idle_task2, "idle_task2", 1024 * 2, NULL, 10, NULL);
}

// -----------------------------------------------------------------------
// CPU measure public interface

int    get_cpu_usage()

{
    // In case it is not running ...
    if(!usage_inited)
        {
        init_cpu_measure();
        vTaskDelay(1000 / portTICK_RATE_MS);
        }
    int ret = (1000 - new_cnt) / 10;
    // Only return prettty
    if(ret < 0)
        ret = -1;

    if(ret >= 100)
        ret = -1;

    // Attempt recovery
    if(ret == -1)
        {
        idle_cnt = 0;
        new_cnt = 0;
        }

    return(ret);
}

// EOF
