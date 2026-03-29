
/* =====[ access point template project ]=================================

   File Name:       common.c

   Description:

   Revisions:

      REV       DATE               BY          DESCRIPTION
      ----  -----------         ----------      -------------------------
      0.00  Tue 10.Mar.2026     Peter Glen      Initial version.

   ======================================================================= */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_now.h>
#include <lwip/err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include "nvs.h"

// -----------------------------------------------------------------------
// NVS time calculation
// -----------------------------------------------------------------------

// Secs to a year
//      60 * 60 * 24 * 365 = 31,536,000

// Every second:
//      "100000 / (60 * 60 * 24)" => 1.1574074 days
//
// Every minute:
//      "100000 / (60*24)" -> 69.444444 days

// Multiply factor  (calculated from available memory)
//    1516680 / 128 =  1849.062

// Adjusted by multiply factor:

// Every second:
//      "1849 * 100000 / (60 * 60 * 24 * 365)" => 5.8631405 years
//
// Every minute:
//      "1849 * 100000 / (60 * 24 * 365)" -> 351.78843 years

static char *TAG= "ap_nvs";

#define NVS_STR_TYPE    1
#define NVS_INT_TYPE    2
#define NVS_LL_TYPE     3
#define NVS_SHORT_TYPE  4

static int inited = 0;

union  payld {
    uint16_t    sval;
    uint64_t    llval;
    uint32_t    val;
    char        *str;
};

typedef struct _nvs_queue
{
    uint8_t     type;
    char        name[16];
    union payld pyl;
}  nvs_queue;

//static xQueueHandle submit_queue;
static QueueHandle_t submit_queue;

//static int64_t startx = 0, endx = 0;

static void    write_nvs_short(char *name, int16_t val)

{
    nvs_handle my_handle;
    //err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);

    if (err != ESP_OK)
        {
        printf("Error (%d) opening NVS handle for %s.", err, name);
        }
    else
        {
        err = nvs_set_i16(my_handle, name, val);
        if (err != ESP_OK)
            {
            printf("Error (%d) writing %s to nvs", err, name);
            }
        nvs_commit(my_handle); nvs_close(my_handle);
        }
}

static void    write_nvs_int(char *name, int val)

{
    nvs_handle my_handle;
    //err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);

    if (err != ESP_OK)
        {
        printf("Error (%d) opening NVS handle for %s.", err, name);
        }
    else
        {
        err = nvs_set_i32(my_handle, name, val);
        if (err != ESP_OK)
            {
            printf("Error (%d) writing %s to nvs", err, name);
            }
        nvs_commit(my_handle); nvs_close(my_handle);
        }
}

static void    write_nvs_int64(char *name, int64_t llval)

{
    nvs_handle my_handle;
    //err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);

    if (err != ESP_OK)
        {
        printf("Error (%d) opening NVS handle for %s.", err, name);
        }
    else
        {
        //printf("Writing %s -> %lld\n", name, llval);
        err = nvs_set_i64(my_handle, name, llval);
        if (err != ESP_OK)
            {
            printf("Error (%d) writing %s to nvs", err, name);
            }
        nvs_commit(my_handle); nvs_close(my_handle);
        }

    // Report on time elapsed
    //endx = esp_timer_get_time();
    //printf("NVS time diff %lld\n", endx - startx);
}

static  void    write_nvs_str(char *name, char *val)

{
    //printf("write_nvs_str() '%s' '%s'\n", name, val);

    nvs_handle my_handle;
    //err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);

    if (err != ESP_OK)
        {
        printf("Error (%d) opening NVS handle for %s.", err, name);
        //gl_nvs_err++;
        }
    else
        {
        err = nvs_set_str(my_handle, name, val);
        if (err != ESP_OK)
            {
            printf("Error (%d) writing %s to nvs", err, name);
            //gl_nvs_err++;
            }
        nvs_commit(my_handle); nvs_close(my_handle);
        }
}

static  void    nvs_write_worker(void *pvParameter)

{
    while(1)
        {
        nvs_queue evt;

        while (xQueueReceive(submit_queue, &evt, portMAX_DELAY) == pdTRUE)
            {
            if (evt.type == NVS_INT_TYPE)
                {
                write_nvs_int(evt.name, evt.pyl.val);
                }
            else if (evt.type == NVS_SHORT_TYPE)
                {
                write_nvs_short(evt.name, evt.pyl.sval);
                }
            else if (evt.type == NVS_LL_TYPE)
                {
                write_nvs_int64(evt.name, evt.pyl.llval);
                }
            else if (evt.type == NVS_STR_TYPE)
                {
                write_nvs_str(evt.name, evt.pyl.str);
                free(evt.pyl.str);
                }
            else
                {
                ESP_LOGE(TAG, "Bad type code in worker");
                }
            }
        // Just in case it exits
        vTaskDelay(20 / portTICK_PERIOD_MS);
        }
    vTaskDelete(NULL);  // Should never happen
}

//////////////////////////////////////////////////////////////////////////
// Send decorated data via queue

int     submit_nvs_str(const char *name, const char *strx)

{
    if(!inited)
        init_nvs_writer();

    int ret = 0, newlen = strlen(strx);
    char *data = malloc(newlen + 1);
    if(!data)
        {
        ESP_LOGE(TAG, "Cannot allocate data buffer in submit_nvs_str");
        ret = -1;
        goto endd;
        }
    nvs_queue evt; memset(&evt, '\0', sizeof(evt));
    evt.type = NVS_STR_TYPE;
    strcpy(data, strx);
    strncpy(evt.name, name, sizeof(evt.name));
    evt.pyl.str = data;
    if (xQueueSend(submit_queue, &evt, 1000 / portTICK_PERIOD_MS) != pdTRUE)
        {
        ESP_LOGE(TAG, "Submit to queue failed.");
        free(evt.pyl.str);
        ret = -2;
        }
   endd:
    return ret;
}

//////////////////////////////////////////////////////////////////////////
// Send decorated data via queue

int     submit_nvs_int(const char *name, int valx)

{
    if(!inited)
        init_nvs_writer();

    int ret = 0;
    nvs_queue evt; memset(&evt, '\0', sizeof(evt));
    strncpy(evt.name, name, sizeof(evt.name));
    evt.type = NVS_INT_TYPE;
    evt.pyl.val = valx;
    if (xQueueSend(submit_queue, &evt, 1000 / portTICK_PERIOD_MS) != pdTRUE)
        {
        ESP_LOGE(TAG, "Submit to queue failed.");
        ret = -2;
        }
    return ret;
}

int     submit_nvs_short(const char *name, int16_t valx)

{
    if(!inited)
        init_nvs_writer();

    int ret = 0;
    nvs_queue evt; memset(&evt, '\0', sizeof(evt));
    strncpy(evt.name, name, sizeof(evt.name));
    evt.type = NVS_SHORT_TYPE;
    evt.pyl.sval = valx;
    if (xQueueSend(submit_queue, &evt, 1000 / portTICK_PERIOD_MS) != pdTRUE)
        {
        ESP_LOGE(TAG, "Submit to queue failed.");
        ret = -2;
        }
    return ret;
}

//////////////////////////////////////////////////////////////////////////
// Send decorated data via queue

int     submit_nvs_int64(const char *name, int64_t valx)

{
    //startx = esp_timer_get_time();
    if(!inited)
        init_nvs_writer();

    int ret = 0;
    nvs_queue evt; memset(&evt, '\0', sizeof(evt));
    strncpy(evt.name, name, sizeof(evt.name));
    evt.type = NVS_LL_TYPE;
    evt.pyl.llval = valx;
    if (xQueueSend(submit_queue, &evt, 3000 / portTICK_PERIOD_MS) != pdTRUE)
        {
        ESP_LOGE(TAG, "Submit to queue failed.");
        ret = -2;
        }
    return ret;
}

//////////////////////////////////////////////////////////////////////////
// Send decorated data via queue .. this is converted to string

int     submit_nvs_float(const char *name, float valx)

{
    int ret = 0;

    if(!inited)
        init_nvs_writer();

    char tmp[16]; snprintf(tmp, sizeof(tmp), "%f", valx);
    ret = submit_nvs_str(name, tmp);

    return ret;
}

int     get_nvs_free()

{
    nvs_stats_t nvs_stats;
    esp_err_t err = nvs_get_stats(NULL, &nvs_stats);
    (void)err;
    int ret = nvs_stats.free_entries * 32;
    return ret;
}

void    get_nvs_info()
{
    nvs_stats_t nvs_stats;
    esp_err_t err = nvs_get_stats(NULL, &nvs_stats);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "NVS Statistics:");
        ESP_LOGI(TAG, "Total entries: %u", nvs_stats.total_entries);
        ESP_LOGI(TAG, "Used entries: %u", nvs_stats.used_entries);
        ESP_LOGI(TAG, "Free entries: %u", nvs_stats.free_entries);
        ESP_LOGI(TAG, "Namespace count: %u", nvs_stats.namespace_count);

        // Calculate the flash size based on the number of pages
        // Each NVS page is 4096 bytes (0x1000)
        uint32_t page_size = 4096;
        uint32_t total_flash_size = nvs_stats.total_entries / 126 * page_size; // 126 entries per page
        ESP_LOGI(TAG, "Total NVS flash size: %u bytes", total_flash_size);

    } else {
        ESP_LOGE(TAG, "Failed to get NVS statistics (%s)", esp_err_to_name(err));
    }
}

// -----------------------------------------------------------------------
// NVS needs a delayed write; Initing on demand ...

void    init_nvs_writer()

{
    if(inited)
        return;
    inited = true;

    submit_queue = xQueueCreate(30, sizeof(nvs_queue));
    xTaskCreate(nvs_write_worker, "nvs_write_worker", 3048, NULL, 4, NULL);
}

// EOF
