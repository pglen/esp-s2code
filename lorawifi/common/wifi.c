
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
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "esp_sleep.h"

#include "protocol.h"
#include "common.h"
#include "sere.h"
#include "input.h"
#include "wifi.h"

int     gl_wifi_on = 0;

char    gl_netname[32] = {0, };
char    gl_netpass[32] = {0, };

static void wifi_event_handler(void* arg, esp_event_base_t event_base,    //)
                                    int32_t event_id, void* event_data)
{
    //printf("Wifi Event %d\n", event_id);

    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        //wifi_event_ap_staconnected_t* event =
        //                          (wifi_event_ap_staconnected_t*) event_data;
        //printf("station "MACSTR" join, AID=%d\n",
        //         MAC2STR(event->mac), event->aid);

    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        //wifi_event_ap_stadisconnected_t* event =
        //                            (wifi_event_ap_stadisconnected_t*) event_data;
        //printf("station "MACSTR" leave, AID=%d\n",
        //         MAC2STR(event->mac), event->aid);
    }
}

static  uint64_t  old_gl_last_http = 0;
static  uint64_t  web_break_out = 0;

// -----------------------------------------------------------------------
// See for timeout, restore operation

void    web_task(void *ptr)

{
    //printf("Web server starting .... \n");

    printf("Web server staring.\n");

    esp_wifi_disconnect();
    vTaskDelay(200 / portTICK_PERIOD_MS);

    wifi_sleep_wake(false);
    vTaskDelay(200 / portTICK_PERIOD_MS);

    esp_wifi_deinit();
    vTaskDelay(200 / portTICK_PERIOD_MS);

    wifi_initial(2);
    //start_webserver();

    int cnt = 0;
    web_break_out = 0;

    while(true)
        {
        // Was any activity?
        if (old_gl_last_http != gl_last_http)
            {
            old_gl_last_http = gl_last_http;
            cnt = 0;
            }
        if (cnt >= 120)     // 2 per second * 60 sec
            break;

        if (web_break_out)
            break;

        cnt++;
        gl_alive = 3;
        vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    gl_webon = false;

    printf("Web server stopping .... \n");
    //ESP_ERROR_CHECK( wifi_sleep_wake());

    esp_wifi_disconnect();
    vTaskDelay(200 / portTICK_PERIOD_MS);

    wifi_sleep_wake(false);
    vTaskDelay(200 / portTICK_PERIOD_MS);

    //ESP_ERROR_CHECK( esp_wifi_deinit());
    esp_wifi_deinit();
    vTaskDelay(200 / portTICK_PERIOD_MS);

    if(gl_iam_battery)
        wifi_initial(0);
    else
        wifi_initial(1);

    if(gl_iam_battery)
        ls3.led_ontime = 20,  ls3.led_cycletime = 40, ls3.led_cyclecnt = 1;
    else
        ls2.led_ontime = 20,  ls2.led_cycletime = 40, ls2.led_cyclecnt = 1;

    esp_restart();

    vTaskDelete(NULL);
}

// -----------------------------------------------------------------------

void    start_webpage()

{
    if(gl_iam_battery)
        ls3.led_ontime = 2000,  ls3.led_cycletime = 3000, ls3.led_cyclecnt = -1;
    else
        ls2.led_ontime = 2000,  ls2.led_cycletime = 3000, ls2.led_cyclecnt = -1;

    if(gl_webon)
        {
        web_break_out = true;
        //printf("Web server aborting .... \n");
        return;
        }

    // Disable all others
    gl_stop_pair = true;
    gl_stop_listen = true;
    vTaskDelay(200 / portTICK_PERIOD_MS);

    gl_webon = true;
    xTaskCreate(web_task, "web_task", 4024, NULL, 2, NULL);
}

uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

//typedef void (*wifi_promiscuous_cb_t)(void *buf, wifi_promiscuous_pkt_type_t type)

#if 0
static  void    callb(void *buf, wifi_promiscuous_pkt_type_t type)

{
    int len = 128;
    //printf("Callback %s\n", (char*)buf);

    //char *ptr = malloc(len);
    //dump_str(buf, 32, ptr, len);
    //printf("%s\n", ptr);
    //free(ptr);
}
#endif

static int wifi_inited = false;

// -----------------------------------------------------------------------
// Initialize WiFi subsys

void    wifi_initial(int startap)

{
    //int sss = get_ms();
    //printf("wifi_initial %d %d-start  ", startap, sss);

    if(!wifi_inited)
        {
        wifi_inited = true;

        // Initialize NVS
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES
                || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
            {
            ESP_ERROR_CHECK( nvs_flash_erase() );
            ret = nvs_flash_init();
            }
        //ESP_ERROR_CHECK( ret );
        //printf("%d-nvs  ", get_ms()-sss);

        ESP_ERROR_CHECK(esp_event_loop_create_default());
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler, NULL));
        ESP_ERROR_CHECK(esp_netif_init());
        esp_netif_create_default_wifi_ap();
        esp_netif_create_default_wifi_sta();
        }

    //printf("%d-netif  ", get_ms()-sss);

    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = false;

    //if(startap != 2)
    //    cfg.beacon_max_len = 0;

    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    //printf("%d-ini   ", get_ms()-sss);

    // DevkitC Version 4 went into power save, stop it
    //ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MAX_MODEM));
    //printf("%d-ps   ", get_ms()-sss);

    //ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_AP) );
    //ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_APSTA) );

    //printf("%d-ap   ", get_ms()-sss);

    // Boot
    if(startap == 1)
        {
        wifi_config_t config; memset(&config, 0, sizeof(wifi_config_t));

        //config.ap.channel = 2;
        //sprintf((char*)config.ap.ssid, "IOCOM_%x%x%x", self_mac[3], self_mac[4], self_mac[5] );

        //config.ap.beacon_interval = 1000;

        GET_MAC(self_mac);
        sprintf(gl_netname, "IO-COM4-%X%X", self_mac[4], self_mac[5] );
        sprintf(gl_netpass, "%s", "12345678");

        memcpy(config.ap.ssid, gl_netname, sizeof(gl_netname));
        memcpy(config.ap.password, gl_netpass, sizeof(gl_netpass));

        config.ap.authmode = WIFI_AUTH_WPA2_PSK;
        config.ap.max_connection = 4;

        config.ap.ssid_hidden = true;

        //printf("Web name: '%s' pass '%s'\n",  gl_netname, gl_netpass);

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &config));

        //ESP_ERROR_CHECK( esp_wifi_set_protocol(ESP_IF_WIFI_AP,
        //            WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
        }
    else if(startap == 2)
        {
         wifi_config_t config; memset(&config, 0, sizeof(wifi_config_t));

        //config.ap.channel = 2;
        //sprintf((char*)config.ap.ssid, "IOCOM_%x%x%x", self_mac[3], self_mac[4], self_mac[5] );

        //if(gl_netname[0] == '\0')
            {
            GET_MAC(self_mac);
            sprintf(gl_netname, "IOCOM4-%X%X", self_mac[4], self_mac[5] );
            sprintf(gl_netpass, "%s", "12345678");
            }
        memcpy(config.ap.ssid, gl_netname, sizeof(gl_netname));
        memcpy(config.ap.password, gl_netpass, sizeof(gl_netpass));

        config.ap.authmode = WIFI_AUTH_WPA2_PSK;
        config.ap.max_connection = 4;
        //config.ap.ssid_hidden = true;
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &config));

        //ESP_ERROR_CHECK( esp_wifi_set_protocol(ESP_IF_WIFI_AP,
        //            WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
        }
    else
        {
        ESP_ERROR_CHECK( esp_wifi_set_protocol(ESP_IF_WIFI_AP, WIFI_PROTOCOL_LR) );
        }

    //printf("%d-conf   ", get_ms()-sss);

    //ESP_ERROR_CHECK( esp_wifi_set_channel(2, 3));

    ESP_ERROR_CHECK( wifi_sleep_wake(true));
    //printf("%d-wstart   ", get_ms()-sss);

    //esp_wifi_set_promiscuous_rx_cb(callb);
    //esp_wifi_set_promiscuous(true);
    //printf("%d-prom   ", get_ms()-sss);

    if(startap == 0 || startap == 1)
        {
        ESP_ERROR_CHECK( esp_now_init() );
        //printf("%d-now   ", get_ms()-sss);
        }

    // Check how long to SW on / off
    //uint64_t  ss2 = esp_timer_get_time();
    //ESP_ERROR_CHECK( wifi_sleep_wake());
    //uint64_t  ss3 = esp_timer_get_time();
    //ESP_ERROR_CHECK( wifi_sleep_wake());
    //uint64_t  ss4 = esp_timer_get_time();
    //ESP_ERROR_CHECK( wifi_sleep_wake());
    //uint64_t  ss5 = esp_timer_get_time();
    //ESP_ERROR_CHECK( wifi_sleep_wake());
    //printf("%lld %lld %lld %lld %lld\n", ss, ss2, ss3, ss4, ss5);
    //return;

    if(startap == 0 || startap == 1)
        {
        /* Add broadcast peer information to peer list. */
        esp_now_peer_info_t peer;
        memset(&peer, 0, sizeof(esp_now_peer_info_t));
        peer.channel = 0;
        peer.ifidx = ESP_IF_WIFI_AP;
        peer.encrypt = false;
        memcpy(peer.peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
        ESP_ERROR_CHECK( esp_now_add_peer(&peer) );
        //printf("%d-peer  \n", get_ms()-sss);
        }

    //printf("peer info %d\n", sizeof(esp_now_peer_info_t));   // Final act
    //printf("\n");
    if(startap == 2)
        {
        //printf("Web name: '%s' pass '%s'\n",  gl_netname, gl_netpass);
        }
    //printf("%d-wifi on\n", get_ms()-sss);
    gl_wifi_on = true;
}

static int woken = 0;
int     gl_wifi_trans  = 0;

// Wake wifi on demand; simple duplicate preventor logic

int     wifi_sleep_wake(int wakeflag)

{
    int ret = ESP_OK;

    if(wakeflag)
        {
        if(!woken)
            {
            woken = true;
            //gl_wifi_trans = true;
            ret = esp_wifi_start();
            vTaskDelay(20 / portTICK_PERIOD_MS);
            //gl_wifi_trans = false;
            }
        }
    else
        {
        if(woken)
            {
            woken = false;
            //gl_wifi_trans = true;
            ret = esp_wifi_stop();
            vTaskDelay(20 / portTICK_PERIOD_MS);
            //gl_wifi_trans = false;
            }
        }
    return ret;
}

// -----------------------------------------------------------------------

void    wifi_wait_ready()

{
    // Wait for WiFi to become available
    for(int aa = 0; aa < 5; aa++)
        {
        if(gl_wifi_on)
            break;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        }
}

// EOF

