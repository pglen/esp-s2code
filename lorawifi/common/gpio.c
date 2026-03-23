
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
#include <sys/param.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "esp_system.h"
#include "esp_timer.h"
//#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"

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

//#include <mbedtls/aes.h>

#include "protocol.h"
#include "common.h"
#include "sere.h"
#include "input.h"
#include "wifi.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#define CHIP_NAME "ESP32"
#endif

uint8_t     portarr[4] = {INPUT1, INPUT2, INPUT3, INPUT4};

//static char *TAG = "IO4_gpio";
static xQueueHandle gpio_evt_queue = NULL;

typedef struct _ButtonPress

{
    uint64_t btime;
    uint8_t  button;
    uint8_t  value;
} ButtonPress;

static uint64_t last = 0;
static  int     old_pair_lev = -1;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    // See if it is our port
    //int found = 0;
    //for(int aa = 0; aa < sizeof(portarr); aa++)
    //    {
    //    if(portarr[aa] == ((int)arg))
    //        found = true;
    //    }
    //if(!found)
    //    {
    //    //printf("non interrupt IO[%d] \n", (int)(arg));
    //    }

    // Store parameters for the press
    ButtonPress bp;
    bp.btime = esp_timer_get_time();
    bp.button = (uint8_t)((int)arg);    // Double cast to prevent warning
    bp.value = gpio_get_level((int)arg);


    // If less than 10 ms, discard  (measured: on test button)
    if(bp.btime - last > 10000)
        {
        xQueueSendFromISR(gpio_evt_queue, &bp, NULL);
        last = bp.btime;
        }
    //printf("interrupt GPIO[%d] \n", (int)(*arg));
}

// -----------------------------------------------------------------------
//

static int old_mask = 0;

int     get_butt_masks()

{
    // Collect button states
    int mask = 0;
    for(int aa = 0; aa < sizeof(portarr); aa++)
        mask |=  rtc_gpio_get_level(portarr[aa]) << aa;
    return mask;
}

static  void    gpio_mon(void* arg)

{
    while(1)
        {
        int mask = get_butt_masks();
        if(mask != old_mask)
            {
            //printf("Mask change %d\n", mask);
            old_mask = mask;
            gl_presscnt++;
            trans_one_cycle();
            }
        vTaskDelay(20 / portTICK_PERIOD_MS);
        }
}

// -----------------------------------------------------------------------

static int     old_mask_val[12] = {0, };

static  void    gpio_task (void* arg)

{
    ButtonPress bp;

    for(;;)
        {
        if(xQueueReceive(gpio_evt_queue, &bp, portMAX_DELAY))
            {
            int lev = gpio_get_level(bp.button);
            //printf("%lld GPIO[%d] intr, val: %d new: %d final: %d\n", bp.btime,
            //                                bp.button, bp.value, val, gpio_get_level(bp.button));
            //printf("%d GPIO[%d] val: %d\n", get_ms(), bp.button, lev);
            if(bp.button ==  CONF_BUTT)
                {
                //printf("Config GPIO[%d] val: %d val-get: %d old: %d\n", bp.button, bp.value, lev, old_pair_lev);
                // On the DOWN wards trajectory
                //if(gl_iam_battery)
                    {
                    key_feeder();
                    }
                //printf("lev=%d old_pair_lev=%d\n", lev, old_pair_lev);
                old_pair_lev = lev;
                }
            else
                {
                if(!gl_webon)
                    {
                    gl_presscnt++;
                    //printf("%d GPIO[%d] val: %d\n", get_ms(), bp.button, bp.value);

                    int idx = -1;
                    for(int aa = 0; aa < sizeof(portarr); aa++)
                        {
                        if(portarr[aa] == bp.button)
                            {
                            idx = aa;
                            break;
                            }
                        }
                    if(idx >= 0)
                        {
                        if(old_mask_val[idx] != bp.value)
                            {
                            //printf("%d XX GPIO[%d] val: %d\n", get_ms(), bp.button, bp.value);
                            old_mask_val[idx] = bp.value;
                            trans_one_cycle();
                            }
                        else
                            {
                            vTaskDelay(10 / portTICK_PERIOD_MS);
                            }
                        }
                    }
                }
            }
        vTaskDelay(10 / portTICK_PERIOD_MS);
        }
}

// -----------------------------------------------------------------------

void    init_gpios()

{
    gpio_install_isr_service(0);

    gpio_evt_queue = xQueueCreate(10, sizeof(ButtonPress));

    // Set all inputs to operate as rtc
    for(int aa = 0; aa < sizeof(portarr); aa++)
        init_rtc_in(portarr[aa]);

    //for(int aa = 0; aa < sizeof(portarr); aa++)
    //    {
    //    gpio_set_intr_type(gpio, GPIO_INTR_ANYEDGE);
    //    gpio_isr_handler_add(portarr[aa], gpio_isr_handler, (void*)((int)portarr[aa]));
    //    gpio_intr_enable(gpio);
    //    }

    // Also add config port
    init_rtc_in(CONF_BUTT);

    //gpio_isr_handler_add(CONF_BUTT, gpio_isr_handler, (void*)((int)CONF_BUTT));
    //xTaskCreate(gpio_task, "gpio_task ", 3048, NULL, 8, NULL);
    (void)gpio_isr_handler;
    (void)gpio_task;

    xTaskCreate(gpio_mon, "gpio_mon ", 3048, NULL, 4, NULL);
    (void)gpio_mon;
}

