
/* =====[ ap.sess ]========================================================

   File Name:       main.c

   Description:     Functions for main.c

   Revisions:

      REV   DATE                BY              DESCRIPTION
      ----  -----------         ----------      --------------------------
      0.00  Tue 03.Mar.2026     Peter Glen      Initial version.

   ======================================================================= */

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <linenoise/linenoise.h>
#include <argtable3/argtable3.h>
#include <esp_console.h>
#include <esp_timer.h>
#include <esp_https_server.h>
#include <esp_http_server.h>
#include <esp_tls.h>

#include <lwip/err.h>
#include <lwip/sys.h>
#include <esp_heap_caps.h>
#include <driver/gpio.h>

#include "cJSON.h"
#include "lora.h"
#include "leds.h"

#include "../common/protocol.h"
#include "../common/utils.h"
#include "../common/common.h"
#include "../common/wifi.h"
#include "../common/nvs.h"
#include "../common/strlib.h"
#include "../common/comline.h"
#include "../common/packets.h"
#include "../common/comline.h"
#include "../common/httpd.h"

static const char *TAG = "lorawifi";

#define PROG_VER    "1.0"
#define PROG_DATE   "Sat 04.Apr.2026"

SemaphoreHandle_t iSemaphore  = NULL;
SemaphoreHandle_t hSemaphore  = NULL;
// This protects the LORA subsystem
SemaphoreHandle_t sSemaphore  = NULL;

//#define RET_IFZERO(valx, retval) if ((valx)) == 0) return((retval));

#if CONFIG_ESP_GTK_REKEYING_ENABLE
#define EXAMPLE_GTK_REKEY_INTERVAL CONFIG_ESP_GTK_REKEY_INTERVAL
#else
#define EXAMPLE_GTK_REKEY_INTERVAL 0
#endif

char    gl_buff2[256];
char    gl_version[48]  = "";
int     gl_recala       = 0;
int     gl_sentprog     = 0;

char    gl_statx[32]    = "Init";
char    gl_spread[32]   = "";
char    gl_bwidth[32]   = "";
char    gl_txpower[32]  = "";
char    gl_txfreq[32]   = "";
char    gl_corrfreq[32] = "";
char    gl_deftren[32]  = "";
char    gl_ack[16]      = "";
int     gl_prom         = 0;
char    gl_curr_tr[32] = "0";
char    *gl_sentbuff = NULL;
int     gl_sendtrench = 0;
int     gl_rssi = 0;
int     gl_freq_err = 0;
int     gl_update = 0;
int     gl_update2 = 0;
double  gl_ppm = 0;
double  gl_devi = 0;

// Read var from nvs, if none, set default

static void read_set_def(nvs_handle handle, char *key, char *val, int maxlen, char *defval)
{
    unsigned int olen = maxlen;
    esp_err_t err = nvs_get_str(handle, key, val, &olen);
    if (err != ESP_OK || val[0] == '\0')
        {
        //ESP_LOGI(TAG, "Warn (%d) reading NVS %s", err, key, defval);
        strncpy(val, defval, maxlen);
        err = nvs_set_str(handle, key, defval);
        if (err != ESP_OK)
            ESP_LOGI(TAG, "ERR (%d) writing default NVS %s", err, key, defval);
        //printf("nvs def: %s -> '%s'\n", key, val);
        }
    else
        {
        //printf("nvs read: %s -> '%s'\n", key, val);
        }
}

void    read_nvs_hist()

{
    nvs_handle my_handle;
    esp_err_t err2 = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err2 != ESP_OK) {
        ESP_LOGE(TAG, "Error (%d) opening NVS handle!", err2);
        goto err3;
        }
    int16_t curr_hist = 0;
    err2 = nvs_get_i16(my_handle, histx, &curr_hist);

    char head[24]; char strx[256];
    // Load all perm hist
    for (int aa = 0; aa < NVS_WRAP; aa++)
        {
        int now = (curr_hist + aa) % NVS_WRAP;
        unsigned int olen = sizeof(strx);
        snprintf(head, sizeof(head),  "head_%d", now);
        nvs_get_str(my_handle, head, strx, &olen);
        //printf("read hist: %s '%s'\n", head, strx);
        if(strx[0] != '\0')
            add_hist(strx, true);
        }
  err3:
    nvs_close(my_handle);
}

// Read / set default values
void    read_nvs_vars()
{
    char netname[64];
    nvs_handle my_handle;

    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        {
        printf("Error (%d) opening NVS handle.", err);
        return;
        }

    GET_MAC(self_mac);
    snprintf(netname, sizeof(netname),
                "LoraWiFi-%02X%02X", self_mac[4], self_mac[5]);

    read_set_def(my_handle, "netname",  gl_netname, sizeof(gl_netname), netname);
    read_set_def(my_handle, "netpass",  gl_netpass, sizeof(gl_netpass), "12345678");

    read_set_def(my_handle, "spread",   gl_spread,    sizeof(gl_spread),   DEF_SPREAD);
    read_set_def(my_handle, "bwidth",   gl_bwidth,    sizeof(gl_bwidth),   DEF_BWIDTH);
    read_set_def(my_handle, "txpower",  gl_txpower,   sizeof(gl_txpower),  DEF_POWER);
    read_set_def(my_handle, "txfreq",   gl_txfreq,    sizeof(gl_txfreq),   DEF_FREQ);

    read_set_def(my_handle, "deftrench", gl_deftren, sizeof(gl_deftren),    DEF_TRENCH);

    read_set_def(my_handle, "corrfreq",   gl_corrfreq, sizeof(gl_corrfreq), "0");

    nvs_close(my_handle);
}

#if CONFIG_EXAMPLE_ENABLE_HTTPS_USER_CALLBACK
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
static void print_peer_cert_info(const mbedtls_ssl_context *ssl)
{
    const mbedtls_x509_crt *cert;
    const size_t buf_size = 1024;
    char *buf = calloc(buf_size, sizeof(char));
    if (buf == NULL) {
        ESP_LOGE(TAG, "Out of memory - Callback execution failed!");
        return;
    }

    // Logging the peer certificate info
    cert = mbedtls_ssl_get_peer_cert(ssl);
    if (cert != NULL) {
        mbedtls_x509_crt_info((char *) buf, buf_size - 1, "    ", cert);
        ESP_LOGI(TAG, "Peer certificate info:\n%s", buf);
    } else {
        ESP_LOGW(TAG, "Could not obtain the peer certificate!");
    }

    free(buf);
}
#endif

static void https_server_user_callback(esp_https_server_user_cb_arg_t *user_cb)
{
    ESP_LOGI(TAG, "User callback invoked!");
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
    mbedtls_ssl_context *ssl_ctx = NULL;
#endif
    switch(user_cb->user_cb_state) {
        case HTTPD_SSL_USER_CB_SESS_CREATE:
            ESP_LOGD(TAG, "At session creation");

            // Logging the socket FD
            int sockfd = -1;
            esp_err_t esp_ret;
            esp_ret = esp_tls_get_conn_sockfd(user_cb->tls, &sockfd);
            if (esp_ret != ESP_OK) {
                ESP_LOGE(TAG, "Error in obtaining the sockfd from tls context");
                break;
            }
            ESP_LOGI(TAG, "Socket FD: %d", sockfd);
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
            ssl_ctx = (mbedtls_ssl_context *) esp_tls_get_ssl_context(user_cb->tls);
            if (ssl_ctx == NULL) {
                ESP_LOGE(TAG, "Error in obtaining ssl context");
                break;
            }
            // Logging the current ciphersuite
            ESP_LOGI(TAG, "Current Ciphersuite: %s", mbedtls_ssl_get_ciphersuite(ssl_ctx));
#endif
            break;

        case HTTPD_SSL_USER_CB_SESS_CLOSE:
            ESP_LOGD(TAG, "At session close");
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
            // Logging the peer certificate
            ssl_ctx = (mbedtls_ssl_context *) esp_tls_get_ssl_context(user_cb->tls);
            if (ssl_ctx == NULL) {
                ESP_LOGE(TAG, "Error in obtaining ssl context");
                break;
            }
            print_peer_cert_info(ssl_ctx);
#endif
            break;
        default:
            ESP_LOGE(TAG, "Illegal state!");
            return;
    }
}
#endif


static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
        start_webservers();

    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
        stop_webservers();
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
             //.password = CONFIG_EXAMPLE_WIFI_PASSWORD,
             //.ssid = CONFIG_EXAMPLE_WIFI_SSID,
             //.ssid_len = strlen(CONFIG_EXAMPLE_WIFI_SSID),
             //.channel = CONFIG_ESP_WIFI_CHANNEL,
             .channel = 1,
            .max_connection = CONFIG_ESP_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            //.authmode = WIFI_AUTH_WPA3_PSK,
            //.sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                    .required = true,
            },
#ifdef CONFIG_ESP_WIFI_BSS_MAX_IDLE_SUPPORT
            .bss_max_idle_cfg = {
                .period = WIFI_AP_DEFAULT_MAX_IDLE_PERIOD,
                .protected_keep_alive = 1,
            },
#endif
            .gtk_rekey_interval = EXAMPLE_GTK_REKEY_INTERVAL,
        },
    };
    if (strlen(CONFIG_EXAMPLE_WIFI_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    read_nvs_vars();

    memcpy(wifi_config.ap.ssid, gl_netname, sizeof(gl_netname));
    memcpy(wifi_config.ap.password, gl_netpass, sizeof(gl_netpass));

    //printf("Wifi Name '%s' -> '%s'\n",
    //                wifi_config.ap.ssid, wifi_config.ap.password);

    //ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());

    //ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
    //         EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s channel:%d",
             wifi_config.ap.ssid, wifi_config.ap.channel);
}

static  void recv_task (void* arg)
{
    int cntR = 0;
    char    cstr[16], hstr[16];
    char    gl_buffer[256];

    for(;;) {
        long    corr = atofx(gl_corrfreq);
        //printf("corr %ld\n", corr);
        TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
        double orig = lora_get_frequency();
        lora_set_frequency(orig + corr);
        lora_receive();
        gl_buffer[0] = '\0';
        int reclen = lora_receive_packet((uint8_t*)gl_buffer, sizeof(gl_buffer));
        lora_set_frequency(orig);
        GIVE_SEMA(sSemaphore);

        if (reclen == 0)
            {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
            }
        gl_buffer[reclen] = '\0';
        uint16_t hhh, ttt; const char *ptr;
        int paylen = disass_packet(gl_buffer, &hhh, &ttt, &ptr);

        int isOK = check_packet(gl_buffer, reclen);
        if(!isOK)
            {
            printf("Error on packet: '%x'\n", hhh);
            }
        toggle_led(isOK);
        TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
        gl_rssi = lora_packet_rssi();
        gl_freq_err = lora_read_freq_err();
        GIVE_SEMA(sSemaphore);
        gl_devi = trans_freq_deviation(gl_freq_err);
        gl_ppm = ppm_freq_deviation(gl_freq_err);
        blink_led(1, 100, 50, 0);

        if(verbose > 1)
            {
            xStr *ppp = xstr_sprintf(
            "Recvd: %d reclen: %d paylen: %d rssi: %d "
            "cnt: %d check: %d devi: %.0f ppm: %.0f",
                        cntR, reclen, paylen, gl_rssi,
                            cntR, isOK, gl_devi, gl_ppm);
            printf("%s\n", ppp->str);
            xstr_destroy(ppp);
            cntR++;

            xStr *sss = xstr_dumpbuff(ptr, paylen);
            printf("'%s'\n", sss->str);
            xstr_destroy(sss);
            }

        printf("Received: hhh=%x ttt=%d '%s'\n", hhh, ttt, ptr);

        snprintf(cstr, sizeof(cstr), "%d", ttt);
        xStr *sss = xstr_create(0);
        xstr_padbr(sss, &gl_buffer[5], "<br>", 70);

        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "reply", sss->str);
        int16_t sumx = lora_chksum(sss->str, sss->length);
        xstr_destroy(sss);
        snprintf(hstr, sizeof(hstr), "%04x", sumx & 0xffff);
        //printf("Chash %s\n", shstr);
        //printf("cstr: '%s' hstr: '%s'\n", cstr, hstr);
        cJSON_AddStringToObject(root, "chan", cstr);
        cJSON_AddStringToObject(root, "ack", gl_ack);
        cJSON_AddStringToObject(root, "hash", hstr);
        cJSON_AddNumberToObject(root, "isok", isOK);
        cJSON_AddNumberToObject(root, "rssi", gl_rssi);
        cJSON_AddNumberToObject(root, "freq", gl_freq_err);
        cJSON_AddNumberToObject(root, "devi", gl_devi);
        char *buff3 = cJSON_Print(root);
        cJSON_Delete(root);
        add_hist(buff3, false);
        free(buff3);

        gl_update2 = 1;
        gl_update = 1;
        gl_recala = 1;

        GIVE_SEMA(hSemaphore);

        //snprintf(gl_statx, sizeof(gl_statx), "%s", "Done");
        //vTaskDelay(1000 / portTICK_PERIOD_MS);
        //snprintf(gl_statx, sizeof(gl_statx), "%s", "Idle ...");
        //vTaskDelay(1000 / portTICK_PERIOD_MS);
        //printf("Sent idle.\n");

        vTaskDelay(pdMS_TO_TICKS(100));
        }
}

static  void trans_task (void* arg)
{
    TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
    lora_hw_init();         // Initialize LoRa (pins configured in menuconfig)
    init_lora_common();     // Send / Recv common config
    GIVE_SEMA(sSemaphore);

    while(true)
        {
        //printf("Before semaphore release cycle.\n");
        TAKE_SEMA(iSemaphore, TAG, portMAX_DELAY);
        //printf("%lld ms - After semaphore release cycle.\n",
        //                    esp_timer_get_time()/1000);
        //vTaskDelay(1000 / portTICK_PERIOD_MS);
        blink_led(1, 100, 50, 0);
        snprintf(gl_statx, sizeof(gl_statx), "%s", "Sending ...");
        send_payload(gl_sentbuff, gl_sendtrench);
        free(gl_sentbuff);
        gl_sentbuff = NULL;
        gl_update2 = 1;
        gl_update = 1;
        snprintf(gl_statx, sizeof(gl_statx), "%s", "Done");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        snprintf(gl_statx, sizeof(gl_statx), "%s", "Idle ...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        //blink_cnt = 2;
        blink_led(1, 0, 30, 0);
        gl_update2 = 0;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        }
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
        }
    ESP_ERROR_CHECK(ret);

    // See if button held, clear flash
    gpio_num_t gpio_num = 1;
    gpio_input_enable(gpio_num);
    gpio_pullup_en(gpio_num);
    int lev = gpio_get_level(gpio_num);
    //printf("GPIO level %d %d\n", gpio_num, lev);
    if(lev == 0)
        {
        printf("Clearing nvram ...\n");
        //nvs_flash_erase();
        //ret = nvs_flash_init();
        nvs_handle my_handle;
        esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
        if (err != ESP_OK)
            {
            printf("Error on erasing keys %d\n", err);
            goto err3;
            }
        nvs_erase_key(my_handle, "netname");
        nvs_erase_key(my_handle, "netpass");
        nvs_erase_key(my_handle, "spread");
        nvs_erase_key(my_handle, "bwidth");
        nvs_erase_key(my_handle, "txpower");
        nvs_erase_key(my_handle, "txfreq");
        nvs_erase_key(my_handle, "deftrench");
        nvs_erase_key(my_handle, "corrfreq");
        nvs_close(my_handle);
      err3:
          ;
        }

    snprintf(gl_version, sizeof(gl_version), "Version: %s Build: %s", PROG_VER, PROG_DATE);

    //ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
    configure_led();

    CREATE_SEMA(iSemaphore); TAKE_SEMA(iSemaphore, TAG, portMAX_DELAY);
    CREATE_SEMA(hSemaphore); CREATE_SEMA(sSemaphore);

    printf("Initial mem = %ld\n",  esp_get_free_heap_size());
    read_nvs_hist();
    gl_sendtrench = atoi(gl_deftren);
    printf("Entering main loop for '%s'\n", gl_netname);
    start_console("LoraWifi>");
    xTaskCreate(&trans_task, "trans_task", 3048, NULL, 15, NULL);
    xTaskCreate(&recv_task, "recv_task", 3048, NULL, 15, NULL);
    //get_nvs_info();
    blink_led(3, 0, 0, 50);
    //pulse_led(40, 0, 0, 55);
    int cnt = 0;
    while(1)
        {
        //uint32_t ttt = esp_timer_get_time()/1000;
        //printf("time: %ld ms porttick: %ld\n", ttt, portTICK_PERIOD_MS);

        //if(cnt % 12 == 0)
        //    printf("Integ %d\n", heap_caps_check_integrity(MALLOC_CAP_DEFAULT, true));
        //toggle_led(0);

        if(cnt % 4 == 0)
            {
            if(verbose > 2)
                printf("Free mem: %ld bytes\n", esp_get_free_heap_size());
            if(verbose > 3)
                printf("Free nvs: %d bytes\n", get_nvs_free());

            if(gl_recala)
                {
                //pulse_led(40, 0, 55, 0);
                blink_led(2, 0, 50, 10);
                }
            }
        cnt++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
}

// EOF
