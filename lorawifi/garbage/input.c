
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

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>

#include <esp_system.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <nvs_flash.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_now.h>
#include <esp_crc.h>
#include <esp_sleep.h>
#include <esp_timer.h>

#include "protocol.h"
#include "common.h"
//#include "sere.h"
#include "input.h"
#include "wifi.h"

#define     TEST_KEYS 1
//#define     DEBUG_KEYS 1

// -----------------------------------------------------------------------
// This many milliseconds before re-scanning switches

#define     GPIO_INPUT_WAIT         63
#define     GPIO_INPUT_HOLD         20      // Input wait key scans (above) before hold

// We did hundred ms, but the printouts where so even .. so we mixed it up

#define     GPIO_SUPER_HOLD        100      // Input wait scans for super long
#define     GPIO_SUPER_LONG_HOLD   200      // Input wait .... real long

#define     NUM_PUSHBUTT (sizeof(switch_states) / sizeof(int))

//
// This is a double (triple ..) click speed. Too low will
// prevent the user from registering it, too high will appear sluggish
// we attempted to mimic the timing of the computer mouse
//
// 200 is too fast, 500 leads to too much delay

#define    DOUBLE_CLICK_LIMIT   600

// -----------------------------------------------------------------------

int     switch_states[NUMBER_OF_INS] = {0, };

static int     gl_to_cnt[NUMBER_OF_INS] = {0 };         // Timeout count
static int     gl_sus_cnt[NUMBER_OF_INS] = {0 };        // Sustain count
static int     gl_press_cnt[NUMBER_OF_INS] = {0 };      // Times pressed
static int     gl_last_pr[NUMBER_OF_INS] = {0 };        // Last press time

//////////////////////////////////////////////////////////////////////////
// Worker functions

void    emit_long_press(int inpx)

{
    #ifdef TEST_KEYS
    printf( "long press on %d\n", inpx);
    #endif
}

void    emit_long_long_press(int inpx)

{
    #ifdef TEST_KEYS
    printf( "long long press on %d\n", inpx);
    #endif

    // Action
    if(inpx == 0)
        {
        //printf( "Deep reset %d\n", inpx);
        //deep_reset();
        }

    printf("Erasing flash ...\n");
    ESP_ERROR_CHECK( nvs_flash_erase() );
    vTaskDelay(300 / portTICK_RATE_MS);
    esp_restart();
}

void    emit_super_long_press(int inpx)

{
    #ifdef TEST_KEYS
    printf( "super long press on %d\n", inpx);
    #endif

    //log_erase();
}

static void    all_start(int inpx)

{
    #ifdef DEBUG_KEYS
    printf("all_start %d\n", inpx);
    #endif

    if(inpx == 0)
        {
        ls4.led_ontime = 40,  ls4.led_cycletime = 80, ls4.led_cyclecnt = 1;
        }

    gl_sus_cnt[inpx]  = true;  gl_to_cnt[inpx]  = 0;

    //printf("all_start %d\n", inpx);
    //if(inpx == 1)
    //    set_relay2(0, 1);
}

static void    all_stop(int inpx)

{
    int xxx  = (int)(esp_timer_get_time() / 1000LL);

    #ifdef DEBUG_KEYS
    printf("all_stop %d\n", inpx);
    #endif

    //printf("all_stop %d\n", inpx);
    //if(inpx == 1)
    //    set_relay2(0, 0);

    if(gl_to_cnt[inpx] > GPIO_SUPER_LONG_HOLD)
        {
        gl_to_cnt[inpx] = gl_sus_cnt[inpx] = 0;
        emit_super_long_press(inpx);
        gl_sus_cnt[inpx] = false;
        gl_to_cnt[inpx] = 0;
        }
    else if(gl_to_cnt[inpx] > GPIO_SUPER_HOLD)
        {
        gl_to_cnt[inpx] = gl_sus_cnt[inpx] = 0;
        emit_long_long_press(inpx);
        gl_sus_cnt[inpx] = false;
        gl_to_cnt[inpx] = 0;
        }
    else if(gl_to_cnt[inpx] > GPIO_INPUT_HOLD)
        {
        gl_to_cnt[inpx] = gl_sus_cnt[inpx] = 0;
        emit_long_press(inpx);
        gl_sus_cnt[inpx] = false;
        gl_to_cnt[inpx] = 0;
        }
    else
        {
        // Register single click
        if(gl_press_cnt[inpx] == 0)
            gl_press_cnt[inpx]++;

        if(xxx - gl_last_pr[inpx]  <  DOUBLE_CLICK_LIMIT)
            {
            // The first timeout is a double
            gl_press_cnt[inpx]++;
            //printf( "multi %d\n", gl_press_cnt[inpx]);
            }
        else
            {
            //gl_press_cnt[inpx] = 0;
            }
        gl_last_pr[inpx]  = xxx;
        }
}

//////////////////////////////////////////////////////////////////////////
// Timeout on button click, evaluate

static void    eval_multi(int inpx)

{
    //printf("eval_multi() gl_press_cnt[inpx]=%d inpx=%d\n", gl_press_cnt[inpx], inpx);

    // Any Keys?
    if(gl_press_cnt[inpx] == 0)
        return;

    gl_alive = IOCOM_ALIVE;

    if(gl_press_cnt[inpx] > 4)
        {
        #ifdef TEST_KEYS
        printf( "Penta click %d inp=%d\n", gl_press_cnt[inpx], inpx);
        #endif
        delayed_reboot(200);
        }
    else if(gl_press_cnt[inpx] > 3)
        {
        #ifdef TEST_KEYS
        printf( "Quad click %d inp=%d\n", gl_press_cnt[inpx], inpx);
        #endif
        //reset_chip();
        start_webpage();
        }
    else if(gl_press_cnt[inpx] > 2)
        {
        #ifdef TEST_KEYS
        printf( "Triple click %d inp=%d\n", gl_press_cnt[inpx], inpx);
        #endif
        start_listening();
        }
    else if(gl_press_cnt[inpx] > 1)
        {
        #ifdef TEST_KEYS
        printf( "Double click %d inp=%d\n", gl_press_cnt[inpx], inpx);
        #endif
        start_pairing();
        }
    else if(gl_press_cnt[inpx] > 0)
        {
        #ifdef TEST_KEYS
        printf( "Single click %d inp=%d\n", gl_press_cnt[inpx], inpx);
        #endif
        }
    gl_press_cnt[inpx] = 0;
}

//////////////////////////////////////////////////////////////////////////
// Flip flop state variable. Only flip if changed between iterations.
//
// Parameters:  ddefx -> defines for GPIO
//              inpx-> Input channel number
//
// THIS IS THE INVERTED VERSION

#define DECL_MON_CODE(ddefx, inpx)                             \
        lev_x = ! gpio_get_level(ddefx);                       \
        if(lev_x == 0) {                                       \
            if(switch_states[inpx] == 1)                       \
                {                                              \
                switch_states[inpx] = 0;                       \
                all_start(inpx);                               \
                }                                              \
            }                                                  \
        else  {                                                \
            if(switch_states[inpx] == 0)                       \
                {                                              \
                switch_states[inpx] = 1;                       \
                all_stop(inpx);                                \
                }                                              \
            }                                                  \

//////////////////////////////////////////////////////////////////////////
// Monitor the GPIO-s for activity.
// The time resolution is 30 ms

static void    input_task(void *pvParameters)

{
    while(1==1)
        {
        int lev_x;
        int xxx  = (int)(esp_timer_get_time() / 1000LL);

        // Macros writing code ... do not complain
        // this replaces 100+ lines of code.
        DECL_MON_CODE(CONF_BUTT,  0);

        // Examine click limits
        for(int bb = 0; bb < NUM_PUSHBUTT; bb++)
            {
            if(xxx - gl_last_pr[bb] > DOUBLE_CLICK_LIMIT)
                {
                // Only eval if it is released
                //if(gl_sus_cnt[bb] == false)
                eval_multi(bb);
                }
            // Count held down buttons on timer
            if(gl_sus_cnt[bb])
                {
                //ESP_LOGI(TAG, "gl_to_cnt=%d ", gl_to_cnt);
                gl_to_cnt[bb]++;
                }
            }

        //#ifdef WATERMARK
        //static int     cnt = 0;
        //if(cnt++ % 10 == 0)
        //    {
        //    ESP_LOGI("TaskStack", "Stack of %s High Watermark: %d",
        //            pcTaskGetTaskName(NULL), (int)uxTaskGetStackHighWaterMark(NULL));
        //    }
        //#endif

        vTaskDelay(GPIO_INPUT_WAIT / portTICK_RATE_MS);
        }
    vTaskDelete(NULL);
}

//////////////////////////////////////////////////////////////////////////
// Input comes here

void    key_feeder()

{
    int     lev_x;
    gl_alive = 3;
    DECL_MON_CODE(CONF_BUTT,  0);
}

//////////////////////////////////////////////////////////////////////////
// Declare initialization code defx -> GPIO from hron_gpio

#define DECL_INIT_CODE(defx)                                    \
            gpioConfig.pin_bit_mask = 1ULL;                     \
            gpioConfig.pin_bit_mask <<= defx;                   \
            gpioConfig.mode = GPIO_MODE_INPUT;                  \
            gpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;        \
            gpioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;    \
            gpioConfig.intr_type = GPIO_INTR_DISABLE;           \
            ESP_ERROR_CHECK(gpio_config(&gpioConfig));          \

//////////////////////////////////////////////////////////////////////////
// Prepare for listening.

void    init_input()

{
    for(int aa = 0; aa < NUMBER_OF_INS; aa++)
        {
        switch_states[aa] = 1;
        }

     // Init GPIO
    gpio_config_t gpioConfig;
    DECL_INIT_CODE(CONF_BUTT);

    xTaskCreate(input_task, "input_task", 4024, NULL, 2, NULL);
}

