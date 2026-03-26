
/* =====[ access point template project ]=================================

   File Name:       common.c

   Description:

   Revisions:

      REV       DATE               BY          DESCRIPTION
      ----  -----------         ----------      -------------------------
      0.00  Tue 10.Mar.2026     Peter Glen      Initial version.

   ======================================================================= */

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "esp_system.h"
//#include "esp_spi_flash.h"
//#include "driver/gpio.h"
//#include "driver/rtc_io.h"

#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "esp_crc.h"

//#include "esp32/ulp.h"
#include "esp_sleep.h"
//#include "esp32/rom/crc.h"

#include "common.h"
#include "protocol.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#define CHIP_NAME "ESP32"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2BETA
#define CHIP_NAME "ESP32-S2 Beta"
#endif

#if 0

void    eval_waker(int waker)

{
    switch(waker)
        {
        case ESP_SLEEP_WAKEUP_EXT0  :
            //printf("Wakeup caused by external signal using EXT0\n");
            break;
        case ESP_SLEEP_WAKEUP_EXT1  :
            //printf("Wakeup caused by external signal using EXT1\n");
            break;
        case ESP_SLEEP_WAKEUP_TIMER  :
            //printf("Wakeup caused by timer\n");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD  :
            //printf("Wakeup caused by touchpad\n");
            break;
        case ESP_SLEEP_WAKEUP_ULP  :
            //printf("Wakeup caused by ULP program\n");
            break;

        default :
            {
            //printf("Wakeup was not caused by deep sleep\n");
            //printf("Hello world!\n");
            /* Print chip information */
            esp_chip_info_t chip_info;
            esp_chip_info(&chip_info);

            printf("%s chip with %d CPU cores WiFi%s%s\n",
                    CHIP_NAME,
                    chip_info.cores,
                    (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
                    (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
            printf("Revision %d, ", chip_info.revision);
            printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
                    ( chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
            }
            break;
        }
}
#endif

// EOF
