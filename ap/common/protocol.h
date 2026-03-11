
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

//include "esp32/ulp.h"
#include "esp_sleep.h"

#include <esp_http_server.h>

#define PEAR_AMBLE  0xaacc
#define PREAMBLE    0xaabb
#define POSTAMBLE   0xccdd
#define DEFTTL     2
#define SITEID     0

#define CONF_BUTTON GPIO_NUM_4

#define BZERO(ptr, len)   memset((ptr), '\0', (len));

#define CMP_MAC(aa, bb)   memcmp((aa), (bb),  ESP_NOW_ETH_ALEN)
#define COPY_MAC(dest, src)  memcpy((dest), (src),  ESP_NOW_ETH_ALEN)
#define ZERO_MAC(aa)      memset((aa), '\0',  ESP_NOW_ETH_ALEN)
#define TEST_ZERO_MAC(aa)    memcmp((aa), zero_mac, ESP_NOW_ETH_ALEN)

#if CONFIG_STATION_MODE
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

// We do most of the semaphore ops even if the semaphore fails ...
// ... predictable data concept

#define CREATE_SEMA(xSemaphore2)                                    \
        vSemaphoreCreateBinary( xSemaphore2 );                      \

#define TAKE_SEMA(xSemaphore2, xtag, ticks)                         \
        if(!xSemaphoreTake( xSemaphore2, ( TickType_t ) ticks ))    \
            {                                                       \
            ESP_LOGE(xtag, "Could not take semaphore.");            \
            }                                                       \

#define GIVE_SEMA(xSemaphore2)                                      \
        xSemaphoreGive( xSemaphore2 );                              \


#define HTTPH     httpd_handle_t

#define SETUPRAND()     vTaskDelay(10 / portTICK_RATE_MS);       \
                        srand(esp_timer_get_time());             \

#define GET_MAC(vvv)                                \
        uint8_t vvv[ESP_NOW_ETH_ALEN];              \
        esp_wifi_get_mac(ESPNOW_WIFI_IF, vvv);      \

#define INIT_TRANS_PACKET(ptr)                      \
        memset((ptr), '\0', sizeof(Transmit));      \
        (ptr)->preamble = PREAMBLE;                 \
        (ptr)->ttl = DEFTTL;                        \
        (ptr)->siteid = SITEID;                     \
        (ptr)->postamble = POSTAMBLE;               \
        (ptr)->replen = 300;                        \


// -----------------------------------------------------------------------
//
// The rational for not encrypting the pre / post is that the packet can
// be verified witout intensive processing;
//

typedef struct _transmit

{
    uint16_t    preamble;
    // encrypted from here -----------------------------------
    uint16_t    rand;
    uint32_t    siteid;
    uint8_t     ttl;
    uint8_t     batt;
    uint16_t    replen;         // Repeat length; msec

    uint32_t    buttons;        // This is thought of as the payload
    uint32_t    buttons2;       // This is thought of as the payload for the future
    uint32_t    flags;          // For future transmission

    uint16_t    sum;
    // encrypted to here -----------------------------------
    uint16_t    postamble;

} Transmit;


#define TRANS_ENC_SIZE(aa)      \
    sizeof( aa.rand) +          \
    sizeof( aa.siteid) +        \
    sizeof( aa.ttl) +           \
    sizeof( aa.batt) +          \
    sizeof( aa.buttons) +       \
    sizeof( aa.flags) +         \
    sizeof( aa.sum)

typedef struct _sender

{
    uint16_t    magic;
    uint16_t    slen;
    uint8_t     *buff;
}  Sender;

// -----------------------------------------------------------------------
// Data sherd between modules

extern uint8_t  broadcast_mac[ESP_NOW_ETH_ALEN];
extern  int     gl_startpoint;

// -----------------------------------------------------------------------
// Prototypes

//HTTPH   start_webserver(void);
void    eval_waker(int waker);
void    init_rel_out(int gpio);

// EOF
