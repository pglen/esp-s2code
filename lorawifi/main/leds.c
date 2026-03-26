
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

#include "esp_log.h"
#include "esp_system.h"
#include "led_strip.h"

//static const char *TAG = "leds";

#define BLINK_GPIO CONFIG_BLINK_GPIO

static uint8_t s_led_state = 0;

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;

static int     blink_cnt = 0;
static int     rr = 0, gg = 50, bb = 0;

int     delay = 40;
int     delay2 = 200;

void    blink_led(int cnt, int  rrr, int ggg, int  bbb)
{
    rr = rrr;
    gg = ggg;
    bb = bbb;

    blink_cnt = cnt;
}

static  void led_task (void* arg)
{
    while(1)
        {
        if(blink_cnt)
            {
            led_strip_set_pixel(led_strip, 0, rr, gg, bb);
            led_strip_refresh(led_strip);
            vTaskDelay(delay / portTICK_PERIOD_MS);

            led_strip_set_pixel(led_strip, 0, 0, 0, 0);
            led_strip_refresh(led_strip);
            vTaskDelay(delay2 / portTICK_PERIOD_MS);
            blink_cnt--;
            }
        vTaskDelay(100 / portTICK_PERIOD_MS);
        }
}

void pulse_led(int ms, int rr, int gg, int bb)
{
    led_strip_set_pixel(led_strip, 0, rr, gg, bb);
    led_strip_refresh(led_strip);
    vTaskDelay(ms / portTICK_PERIOD_MS);
    led_strip_set_pixel(led_strip, 0, 0, 0, 0);
    led_strip_refresh(led_strip);
}

void toggle_led(int okcol)
{
    //printf("toggle\n");
    /* If the addressable LED is enabled */
    if (s_led_state) {
        if(okcol)
            led_strip_set_pixel(led_strip, 0, 0, 36, 0);
        else
            led_strip_set_pixel(led_strip, 0, 36, 0, 0);

        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    } else {
        /* Set all LED off to clear all pixels */

        if(okcol)
            led_strip_set_pixel(led_strip, 0, 0, 0, 0);
        else
            led_strip_set_pixel(led_strip, 0, 0, 0, 36);

        led_strip_refresh(led_strip);
        //led_strip_clear(led_strip);
    }
    s_led_state = ! s_led_state;
}

void configure_led(void)
{
    //ESP_LOGI(TAG, "Configured addressable LED at %d", BLINK_GPIO);
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "unsupported LED strip backend"
#endif
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
    led_strip_refresh(led_strip);

    xTaskCreate(&led_task, "led_task", 1024, NULL, 15, NULL);
}

#elif CONFIG_BLINK_LED_GPIO

void toggle_led(int ok)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

void configure_led(void)
{
    //ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

# endif

