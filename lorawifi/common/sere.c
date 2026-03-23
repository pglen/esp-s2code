
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
#include "esp_timer.h"
#include "esp_random.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "esp_crc.h"

//#include "esp32/ulp.h"
#include "esp_sleep.h"

#include "protocol.h"
#include "common.h"
#include "sere.h"
#include "wifi.h"

// Back link

//#include "../transmitter/main/a2d.h"

static char *TAG = "IO4_sere";

uint32_t        gl_paired[MAX_PAIRED_DEVICES]   = {0, };
int             gl_fresh_pair = 0;
int             gl_listen   = 0;
int             gl_alive = 0;                   // How many ticks to keep it alive
int             gl_stop_pair = 0;
int             gl_stop_listen = 0;

int             gl_packon = 0;

static  int     gl_broad    = 0;

// Password misery:

//char            *gl_pass = "12345678abcdefghijklmn";
// This ugly pass looks like code, so it cannot be guessed where it is at
//char            *gl_pass = "\x12\x4\x56\x78\xab\xcd\xe2\xf";

// This is the standard pass encrypted with '12345678' (code to generate it is below)

char    *gl_pass = "\x17\x4d\xe1\xfc\xdf\x72\xb0\xb2\x8a\xd1\xb3\x60\x30\x71\xb4\xb3\xd0\x21\xab\x03\xf0";

//    char    u_pass[] = "12345678abcdefghijklmn";
//    int     u_len = strlen(u_pass);
//
//    encrdecrx(1, u_pass, u_len, "12345678", 8);
//    printf("Encoded pass:\n");
//    dump_str2(u_pass, u_len);

static QueueHandle_t submit_queue;
static QueueHandle_t recv_queue;

static SemaphoreHandle_t iSemaphore  = NULL;

#define PARTIAL_SLICE  5

static  void    partial_delay(int msecs)

{
    uint64_t last = gl_presscnt;

    for(int aa = 0; aa < msecs / PARTIAL_SLICE; aa++)
        {
        if(last != gl_presscnt)
            {
            //printf("%lld partial broke out of final burst2.\n", esp_timer_get_time());
            break;
            }
        vTaskDelay(msecs / PARTIAL_SLICE);
        }
}

static void  send_packet(Sender *pevt)

{
    //printf("%d send_packet()\n", get_ms());

    for(int aa = 0; aa < NUM_RETRIES; aa++)
        {
        esp_err_t ret2 = esp_now_send(broadcast_mac, pevt->buff, pevt->slen);

        if (ret2 == ESP_OK)
            break;

        //ESP_LOGE(TAG, "Could not send ESPNOW packet %d (%x)\n", ret2, ret2);
        printf("Retry ESPNOW %d %x\n", aa, ret2);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        }

    if(gl_packon)
        gl_packon--;
}

static  void    send_task(void *arg)

{
    while(1)
        {
        Sender evt;
        while (xQueueReceive(submit_queue, &evt, portMAX_DELAY) == pdTRUE)
            {
            //printf("%d dispatched packet\n", get_ms());
            // magic=%x slen=%d\n",  get_ms(), evt.magic, evt.slen);

            if(evt.magic != 0x1234)
                {
                printf("Bad magic on packet (%x)\n", evt.magic);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                continue;
                }
            wifi_wait_ready();
            if(gl_iam_battery)
                {
                // This was saving power, however it introduces uncertainty
                // on communication
                // Wed 05.May.2021 all good now; polling the port fixed it
                wifi_sleep_wake(true);
                //for(int aa = 0; aa < 3; aa++)
                //    {
                //    if(!gl_wifi_trans)
                //        break;
                //    vTaskDelay(10 / portTICK_PERIOD_MS);
                //    }
                }
            // Machine gun of NUM_BURSTS with retry
            for(int aa = 0; aa < NUM_BURSTS; aa++)
                {
                send_packet(&evt);
                }
           //printf("%d sent packets\n", get_ms());
            if(gl_iam_battery)
                {
                // only sleep after ...
                //if(esp_timer_get_time() > 3LL * 1000LL * 1000LL)
                // Wed 05.May.2021
                wifi_sleep_wake(false);
                }
            free(evt.buff);
            }
        //vTaskDelay(10 / portTICK_PERIOD_MS);
        }
}

static  void    recv_task(void *arg)

{
    while(1)
        {
        Sender evt;
        while (xQueueReceive(recv_queue, &evt, portMAX_DELAY) == pdTRUE)
            {
            printf("%d got recv packet dispatched ptr=%p\n", get_ms(), evt.buff);
            vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        ESP_LOGE(TAG, "Restarted recv_task");
        vTaskDelay(100 / portTICK_PERIOD_MS);
        }
}

static  int    queue_packet(uint8_t const *buff, int plen)

{
    uint8_t *ptr = malloc(plen+1);
    if(!ptr)
        {
        ESP_LOGE(TAG, "Cannot alloc for send buffer");
        return ESP_ERR_NO_MEM;
        }
    memcpy(ptr, buff, plen);
    Sender evt; evt.magic = 0x1234;
    evt.buff = ptr; evt.slen = plen;

    //printf("%d Queue packet\n", get_ms());
    esp_err_t err = xQueueSend(submit_queue, &evt, portMAX_DELAY);

    if (err != pdTRUE)
        {
        printf("Could not queue  ESPNOW packet %d (%x)\n", err, err);
        }

    gl_packon++;

    return (int)err != pdTRUE;
}

int     is_rf_busy()

{
    int ret = uxQueueMessagesWaiting(submit_queue);
    return ret;
}

// -----------------------------------------------------------------------

void    send_trans(int payload, int payload2, int batt)

{
    GET_MAC(self_mac);

    Transmit  spacket;  INIT_TRANS_PACKET(&spacket);

    // This site ID
    spacket.siteid = *((uint32_t *)(&self_mac[2]));

    // Actual transmission data
    spacket.buttons  = payload;
    spacket.buttons2 = payload2;
    spacket.flags    = 0;
    spacket.batt     = batt;
    spacket.replen   = gl_repeat;

    spacket.rand = esp_random();
    spacket.sum = esp_crc16_le(UINT16_MAX, (uint8_t const *)&spacket, sizeof(Transmit));

    encrdecrx(1, &spacket.rand, TRANS_ENC_SIZE(spacket), gl_pass, strlen(gl_pass));

    //printf("%d Submitting packet\n", get_ms());

    esp_err_t ret2 = queue_packet((const uint8_t *)&spacket, sizeof(Transmit));
    if (ret2 != ESP_OK)
        {
        printf("Could not send  ESPNOW packet %d (%x)\n", ret2, ret2);
        vTaskDelay(20 / portTICK_PERIOD_MS);
        }
    //vTaskDelay(10 / portTICK_PERIOD_MS);
    trans_one_cycle();
    static int old_payload = -1;
    if(payload != old_payload)
        {
        printf("%d trans() 0x%x\n",  get_ms(), payload);
        old_payload = payload;
        }

    //printf("%d trans all() 0x%x\n",  get_ms(), payload);

    if(gl_alive < 4)
        gl_alive++;
}

static  void trans_task (void* arg)

{
    while(true)
        {
        //printf("Before semaphore release cycle.\n");
        TAKE_SEMA(iSemaphore, TAG, portMAX_DELAY);
        //printf("%d After semaphore release cycle.\n", get_ms());

        int was_any_pressed = 0, mask = 0;
        uint64_t last_trans = 0;
        uint64_t last = gl_presscnt;

        // Rotate while there are changes
        while(true)
            {
            mask = get_butt_masks();

            // Auto test
            //if(gl_cnt)
            //    mask |= gl_cnt % 2;

            // No button down
            if(mask == 0)
                break;

            was_any_pressed |= mask;

            // Keep rotaing, send if time elapsed
            if (esp_timer_get_time() - last_trans  >= gl_repeat * 1000 || gl_presscnt != last)
                {
                //printf("transmit: %d ms mask: %d (0x%d)\n", get_ms(), mask, mask);
                //send_trans(mask, gl_repeat, V2DV(gl_volts[0]));
                last_trans = esp_timer_get_time();
                if(was_any_pressed)
                    {
                    //printf("volts: %.2f\n", gl_volts[0]);

                    //if(gl_volts[0] > 0 && gl_volts[0] < DV2V(BATT_ELBOW) && gl_iam_battery)
                    //    ls3.led_ontime = 20,  ls3.led_cycletime = 50, ls3.led_cyclecnt = 3;
                    //else
                    //    ls4.led_ontime = 40,  ls4.led_cycletime = 200, ls4.led_cyclecnt = 1;
                    }
                last = gl_presscnt;
                }
            //vTaskDelay(transdelay / portTICK_PERIOD_MS);
            vTaskDelay(20 / portTICK_PERIOD_MS);
            }

        //printf("Ended on was_any_pressed %d mask %d\n", was_any_pressed, mask);

        if(was_any_pressed && mask == 0)
            {
            uint64_t last = gl_presscnt;
            for(int bb = 0; bb < gl_repcnt; bb++)
                {
                //printf("transmit2: %d ms mask: %d (0x%d)\n", get_ms(), mask, mask);
                // New one came in?
                if(last != gl_presscnt)
                    {
                    //printf("%d ms broke out of final burst.\n", get_ms());
                    //send_trans(0, gl_repeat, V2DV(gl_volts[0]));
                    break;
                    }
                //send_trans(0, gl_repeat, V2DV(gl_volts[0]));
                //printf("%lld final burst.\n", esp_timer_get_time());
                partial_delay(gl_repeat / portTICK_PERIOD_MS);
                }
            }
        //printf("Ended off\n");
        // Clear all the semaphore instances
        // TODO --- for now we detect if there was any button pressed

        vTaskDelay(10 / portTICK_PERIOD_MS);
        }
}

// -----------------------------------------------------------------------

static  void    pair_task(void *arg)

{
    GET_MAC(self_mac);
    Transmit  bcast_packet;
    int cnt3 = 0;

    gl_stop_pair = false;

    printf("Started pair broadcast\n");

    while(1)
        {
        if(gl_iam_battery)
            ls3.led_ontime = 200,  ls3.led_cycletime = 500, ls3.led_cyclecnt = -1;
        else
            ls2.led_ontime = 200,  ls2.led_cycletime = 500, ls2.led_cyclecnt = -1;

        INIT_TRANS_PACKET(&bcast_packet);
        bcast_packet.preamble = PEAR_AMBLE;
        bcast_packet.rand = esp_random();
        bcast_packet.replen = gl_repeat;

        //memcpy(&bcast_packet.siteid, &self_mac[2], sizeof(int));
        bcast_packet.siteid = *((uint32_t *)(&self_mac[2]));

        //printf("  Sending pair broadcast %d mac=0x%x\n", cnt3, bcast_packet.siteid);
        bcast_packet.sum = esp_crc16_le(UINT16_MAX, (uint8_t const *)&bcast_packet, sizeof(Transmit));

        encrdecrx(1, &bcast_packet.rand, TRANS_ENC_SIZE(bcast_packet), gl_pass, strlen(gl_pass));

        int ret2 = queue_packet((uint8_t const *)&bcast_packet, sizeof(Transmit));
        if (ret2 != ESP_OK)
            {
            printf("Could not submit ESPNOW packet %d (%x)\n", ret2, ret2);
            }

        gl_alive = 3;
        vTaskDelay(500 / portTICK_PERIOD_MS);

        if(cnt3++ > 20)
            break;

        if(gl_stop_pair)
            break;
        }

    if(gl_iam_battery)
        ls3.led_ontime = 20,  ls3.led_cycletime = 40, ls3.led_cyclecnt = 10;
    else
        ls2.led_ontime = 20,  ls2.led_cycletime = 40, ls2.led_cyclecnt = 10;

    printf("Ended pair broadcast %d\n", cnt3);
    gl_broad = false;

    vTaskDelete(NULL);
}

static  void    list_task(void *arg)

{
    printf("Started listen for pair\n");

    if(gl_iam_battery)
        ls3.led_ontime = 500,  ls3.led_cycletime = 1000, ls3.led_cyclecnt = -1;
    else
        ls2.led_ontime = 500,  ls2.led_cycletime = 1000, ls2.led_cyclecnt = -1;

    gl_stop_listen = false;

    for(int aa = 0; aa < MAX_PAIR_TIME * 2; aa++)
        {
        gl_alive = 3;

        if(gl_stop_listen)
            break;

        if(gl_fresh_pair)
            {
            for(int aa = 0; aa < MAX_PAIRED_DEVICES; aa++)
                {
                if(gl_paired[aa] != 0)
                    {
                    //printf("Got valid pairing 0x%x\n", gl_paired[aa]);
                    break;
                    }
                }
            ls3.led_ontime = 20,  ls3.led_cycletime = 40, ls3.led_cyclecnt = 20;
            gl_alive = 3;
            vTaskDelay(40 / portTICK_PERIOD_MS);
            break;
            }
        //printf("Listening for pair.\n");
        vTaskDelay(500 / portTICK_PERIOD_MS);
        }

    if(gl_iam_battery)
        ls3.led_ontime = 20,  ls3.led_cycletime = 40, ls3.led_cyclecnt = 1;
    else
        ls2.led_ontime = 20,  ls2.led_cycletime = 40, ls2.led_cyclecnt = 1;

    printf("Ended listen for pair.\n");

    gl_listen = false;
    vTaskDelete(NULL);
}

void    start_pairing()

{
    if(gl_broad)
        {
        gl_stop_pair = true;
        return;
        }
    gl_broad = true;
    xTaskCreate(&pair_task, "pair_task", 3048, NULL, 5, NULL);
}

void    start_listening()

{
    if(gl_listen)
        {
        gl_alive = 3;
        gl_stop_listen = true;
        return;
        }

    int avail = -1;
    for(int aa = 0; aa < MAX_PAIRED_DEVICES; aa++)
        {
        if(gl_paired[aa] == 0)
            {
            avail = aa;
            break;
            }
        }
    if(avail < 0)
        {
        printf("Already got valid pairings\n");
        if(gl_iam_battery)
            ls3.led_ontime = 20,  ls3.led_cycletime = 40, ls3.led_cyclecnt = 10;
        else
            ls2.led_ontime = 20,  ls2.led_cycletime = 40, ls2.led_cyclecnt = 10;
        return;
        }
    gl_listen = true;
    gl_fresh_pair = false;
    xTaskCreate(&list_task, "list_task", 3048, NULL, 5, NULL);
}

// -----------------------------------------------------------------------

void    trans_one_cycle()

{
    //printf("trans_one_cycle()\n");

    if(!iSemaphore)
        {
        printf("trans_one_cycle: No sema yet.\n");
        }
    else
        {
        GIVE_SEMA(iSemaphore);
        }
}

void    init_trans()

{
    CREATE_SEMA(iSemaphore); TAKE_SEMA(iSemaphore, TAG, portMAX_DELAY);

    submit_queue = xQueueCreate(20, sizeof(Sender));
    recv_queue = xQueueCreate(20, sizeof(Sender));

    xTaskCreate(trans_task , "trans_task ", 2048, NULL, 6, NULL);
    xTaskCreate(&send_task, "send_task", 3048, NULL, 15, NULL);
    xTaskCreate(&recv_task, "recv_task", 3048, NULL, 15, NULL);
}

// EOF
