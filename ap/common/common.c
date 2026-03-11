
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
#include <ctype.h>
#include <sys/param.h>
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
#include "esp_timer.h"
#include "esp_chip_info.h"
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

int         gl_webon = 0;
uint64_t    gl_last_http = 0;
char        gl_devname[24] = {0, };

int         gl_power  = 20;
int         gl_repeat = 300;
int         gl_repcnt = MAX_NUM_BURSTS;

uint8_t     ledarr[4]  = {LED1, LED2, LED3, LED4 };

uint64_t    gl_presscnt = 0;
int         gl_iam_battery = 0;
int         verbose = 0;

static char *TAG = "IO4_prot";

void    dump_str2(const void *vptr, int len)

{
    char *ptr = malloc(len * 5 + 64);
    //printf("Callback %s\n", (char*)buf);
    if(!ptr)
        return;

    dump_str(vptr, len, ptr, len * 5);
    printf("%s\n", ptr);
    free(ptr);
}

void    dump_str(const void *vptr, int len, char *out, int olen)

{
    int prog = 0;  *out = '\0'; int aa = 0;
    char dd[24];  dd[0] = '\0';
    char *ptr = (char*)vptr;
    char ss[4]; char cc[3];
    char *trailer =  "  | ";
    int tlen = strlen(trailer);

    for(aa = 0; aa < len; aa++)
        {
        char chh = ptr[aa];
        snprintf(ss, sizeof(ss), "%02x ", chh);
        snprintf(cc, sizeof(cc), "%c", isprint(chh) ? chh : '.');

        cc[1] = '\0';
        strcat(dd, cc);
        if((aa % 16) == 15)
            {
            strcat(out, trailer); strcat(out, dd); strcat(out, trailer);
            prog += strlen(dd) + 2 * tlen;
            dd[0] = '\0';
            snprintf(ss, sizeof(ss), "\n");
            }
        strcat(out, ss);

        prog += strlen(ss);

        if(prog > olen - 12)
            break;
        }
    // Padd it
    for(int bb = 0; bb < 15 - (aa % 16); bb++)
        {
        strcat(out, "   ");
        prog += 4;
        }
    strcat(out, trailer);
    prog += tlen;
    strcat(out, dd);
    prog += strlen(dd) + 2;
    strcat(out, "\n");
    prog += 2;
    out[prog] = '\0';
}

int     get_ms()

{
    return (int)(esp_timer_get_time() / 1000LL);
}

#if 0
void    preprocess_string(char* str)

{
    char *p, *q;

    for (p = q = str; *p != 0; p++)
    {
        if (*(p) == '%' && *(p + 1) != 0 && *(p + 2) != 0)
        {
            // quoted hex
            uint8_t a;
            p++;
            if (*p <= '9')
                a = *p - '0';
            else
                a = toupper(*p) - 'A' + 10;
            a <<= 4;
            p++;
            if (*p <= '9')
                a += *p - '0';
            else
                a += toupper(*p) - 'A' + 10;
            *q++ = a;
        }
        else if (*(p) == '+') {
            *q++ = ' ';
        } else {
            *q++ = *p;
        }
    }
    *q = '\0';
}

#endif


Led_Script      ls1 = {0,}, ls2 = {0,}, ls3 = {0,}, ls4 = {0,};
Led_Script      *ls_arr[] = { &ls1, &ls2, &ls3, &ls4 };

// -----------------------------------------------------------------------

static void channel_blink(Led_Script *pls_arr[],  int chan)

{
    int now = get_ms();

    if(pls_arr[chan]->_led_mark == 0)
        pls_arr[chan]->_led_mark = now;

    int elapsed = now - pls_arr[chan]->_led_mark;

    //int cycles = elapsed / pls_arr[chan]->led_cycletime;
    //printf("cycle %d ",  cycles);

    int in_phase = elapsed % pls_arr[chan]->led_cycletime;
    //printf("in_phase %d\n",  in_phase);

    if(in_phase < pls_arr[chan]->led_ontime)
        {
        if(pls_arr[chan]->_old_state == false)
            {
            pls_arr[chan]->_old_state = true;
            gpio_set_level(ledarr[chan], LED_ON);
            }
        }
    else
        {
        if(pls_arr[chan]->_old_state == true)
            {
            pls_arr[chan]->_old_state = false;

            gpio_set_level(ledarr[chan], LED_OFF);
            if (pls_arr[chan]->led_cyclecnt > 0)
                pls_arr[chan]->led_cyclecnt --;

            if(pls_arr[chan]->led_cyclecnt == 0)
                {
                // Clear state
                pls_arr[chan]->_led_mark = 0;
                }
            }
        }
}

// -----------------------------------------------------------------------

static  void    led_task(void *arg)

{
    //ls1.led_ontime = 20,  ls1.led_cycletime = 500, ls1.led_cyclecnt = 10;
    //ls2.led_ontime = 100, ls2.led_cycletime = 500, ls2.led_cyclecnt = 10;
    //ls3.led_ontime = 200, ls3.led_cycletime = 700, ls3.led_cyclecnt = 10;
    //ls4.led_ontime = 500,  ls4.led_cycletime = 900, ls4.led_cyclecnt = 10;

    while(true)
        {
        for(int chan = 0; chan < 4; chan++)
            {
            if(ls_arr[chan]->led_cyclecnt)
                {
                channel_blink(ls_arr, chan);
                }
            }
        vTaskDelay(20 / portTICK_PERIOD_MS);
        }
}

void    init_leds()

{
    gpio_config_t gpioConfig;

    // Init LED GPIO
    for(int aa = 0; aa < sizeof(ledarr); aa++)
        {
        DECL_INIT_IO(ledarr[aa], GPIO_MODE_OUTPUT);
        gpio_set_level(ledarr[aa], 1);
        }
    xTaskCreate(led_task , "led_task ", 2048, NULL, 8, NULL);
}

void    init_rel_out(int gpio)

{
    if(!GPIO_IS_VALID_OUTPUT_GPIO(gpio))
        {
        printf("Invalid GPIO %d", gpio);
        ESP_LOGE(TAG, "Invalid GPIO %d", gpio);
        return;
        }
    gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
    gpio_pulldown_dis(gpio);
    gpio_pullup_dis(gpio);
    gpio_set_level(gpio, 0);
}

// -----------------------------------------------------------------------
// Init RTC gpio

void    init_rtc_in(int gpio)

{
    if(!rtc_gpio_is_valid_gpio(gpio))
        {
        ESP_LOGE(TAG, "Not a valid RTC GPIO");
        return;
        }
    rtc_gpio_init(gpio);
    rtc_gpio_set_direction(gpio, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_en(gpio);   // Note that this needs a pulldown on RTC
    rtc_gpio_pullup_dis(gpio);
}

void    print_chipinfo()

{
    esp_chip_info_t chip_info;  esp_chip_info(&chip_info);

    printf("This is %s chip with %d CPU cores\n", CHIP_NAME, chip_info.cores);
    printf("Silicon revision %d, WiFi%s%s ", chip_info.revision,
                (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
                    (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    //printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
    // (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
}

//////////////////////////////////////////////////////////////////////////
// Delayed reboot task

static void    delayed_reboot_task(void *parm)

{
    ESP_LOGE(TAG, "Delayed reboot ...");

    esp_wifi_disconnect();
    vTaskDelay(100 / portTICK_PERIOD_MS);

    ESP_ERROR_CHECK(esp_wifi_stop());
    vTaskDelay(100 / portTICK_PERIOD_MS);

    ESP_ERROR_CHECK(esp_wifi_deinit());
    vTaskDelay(100 / portTICK_PERIOD_MS);

    vTaskDelay((int)parm / portTICK_PERIOD_MS);

    ESP_LOGE(TAG, "... rebooting ... ");
    esp_restart();
    while(1==1)
        ;
    vTaskDelete(NULL);
}

//////////////////////////////////////////////////////////////////////////
// Delayed reboot

void    delayed_reboot(int wait_ms)

{
    //ESP_LOGE(TAG, "Rebooting ... ");

    // Disconnect everybody: (automatic)
    xTaskCreate(&delayed_reboot_task, "reboot_task", 3024,
                                (void*)wait_ms, 5, NULL);
    vTaskDelay(20 / portTICK_PERIOD_MS);
}

// -----------------------------------------------------------------------

void    print_wake_cause(int waker, char *context)

{
    //printf("eval cause: %d for %s\n", gl_waker, context);

    switch(waker)
        {
        case ESP_SLEEP_WAKEUP_EXT0  :
            printf("Wakeup %s caused by external signal using EXT0\n", context);
            break;

        case ESP_SLEEP_WAKEUP_EXT1  :
            printf("Wakeup %s caused by external signal using EXT1\n", context);
            break;

        case ESP_SLEEP_WAKEUP_TIMER  :
            printf("Wakeup %s caused by timer\n", context);
            break;

        case ESP_SLEEP_WAKEUP_TOUCHPAD  :
            printf("Wakeup %s caused by touchpad\n", context);
            break;

        case ESP_SLEEP_WAKEUP_ULP  :
            printf("Wakeup %s caused by ULP program\n", context);
            break;

        default:
            //printf("Wakeup %s was not caused by deep sleep\n", context);
            break;
        }
}

// -----------------------------------------------------------------------
// Just the first part of the main body

static void prepkey(void *mem, int mlen)

{
    char *mem2 = (char*)mem;
    int mlenx = mlen;

    for(int aa = mlenx/2; aa < mlenx; aa++)
        {
        mem2[aa] += mem2[aa - mlenx/2];
        }
    for(int aa = 0; aa < mlen-1; aa++)
        {
        mem2[aa] += mem2[aa+1];
        }
    for(int aa = 0; aa < mlenx/2; aa++)
        {
        mem2[aa] += mem2[aa + mlenx/2];
        }
    for(int aa = 0; aa < mlen; aa++)
        {
        mem2[aa] += 0x13;
        }
}

static int     encrdecrx2(int mode, void *mem, int mlen, char *keyx, int klen)

{
    char *mem2 = (char*)mem;
    int mlenx = mlen;

    char *keyz = malloc(klen + 1);
    memcpy(keyz, keyx, klen);

    for(int rr = 0; rr < 3; rr++)
        prepkey(keyz, klen);

    //dump_str2(keyz, klen);

    // Make it even
    if (mlenx % 2)
        mlenx--;

    if(mode)
        {
        for(int aa = mlenx/2; aa < mlenx; aa++)
            {
            mem2[aa] ^= mem2[aa - mlenx/2];
            }
        for(int aa = 0; aa < mlen-1; aa++)
            {
            mem2[aa] += mem2[aa+1];
            }
        for(int aa = 0; aa < mlen; aa++)
            {
            mem2[aa] ^= 0x55;
            }
        for(int aa = 0; aa < mlen; aa++)
            {
            mem2[aa] ^= keyz[aa % klen];
            }
        for(int aa = 0; aa < mlenx/2; aa++)
            {
            mem2[aa] += mem2[aa + mlenx/2];
            }
        for(int aa = 0; aa < mlen; aa++)
            {
            mem2[aa] += 0x13;
            }
        }
    else
        {
        for(int aa = 0; aa < mlen; aa++)
            {
            mem2[aa] -= 0x13;
            }
        for(int aa = 0; aa < mlenx/2; aa++)
            {
            mem2[aa] -= mem2[aa + mlenx/2];
            }
        for(int aa = 0; aa < mlen; aa++)
            {
            mem2[aa] ^= keyz[aa % klen];
            }
        for(int aa = 0; aa < mlen; aa++)
            {
            mem2[aa] ^= 0x55;
            }
        for(int aa = mlen-1; aa > 0; aa--)
            {
            mem2[aa-1] -= mem2[aa];
            }
        for(int aa = mlenx/2; aa < mlenx; aa++)
            {
            mem2[aa] ^= mem2[aa - mlenx/2];
            }
        }
    free(keyz);
    return 0;
}

// -----------------------------------------------------------------------

int     encrdecrx(int mode, void *mem, int mlen, char *keyx, int klen)

{
    for(int rounds = 0; rounds < 15; rounds++)
        {
        encrdecrx2(mode, mem, mlen, keyx, klen);
        }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// Increment boot_count. Return new count. Only increment once.

int     inc_bootcount()

{
    static long int boot_count = 0, was_counted = 0;
    esp_err_t err;

    if(was_counted)
        return boot_count;

    nvs_handle my_handle;
    err = nvs_open("lorawifi", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%d) opening NVS handle!", err);
        goto err3;
        }
    err = nvs_get_i32(my_handle, "boot_count", &boot_count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Warn: (%d) getting boot_count NVS value!", err);
        }
    boot_count ++;          // HERE

    err = nvs_set_i32(my_handle, "boot_count", boot_count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%d) writing NVS!", err);
        goto err4;
        }
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%d)commit to NVS!", err);
        goto err4;
        }

    was_counted = true;
    //(void)err;  // Stop warning

  err4:
    // Close
    nvs_close(my_handle);

  err3:
    return boot_count;
}

//
// Relationship between set value and actual value.
//     As follows: {set value range, actual value} =
//  {
//    {[8, 19],8}, {[20, 27],20}, {[28, 33],28}, {[34, 43],34}, {[44, 51],44},
//    {[52, 55],52}, {[56, 59],56}, {[60, 65],60}, {[66, 71],66},
//    {[72, 79],72}, {[80, 84],80}
//  }.
//
//  esp_wifi_set_max_tx_power()
//
// Power mapping:
//
//  {
//    {8, 2}, {20, 5}, {28, 7}, {34, 8}, {44, 11}, {52, 13}, {56, 14}, {60, 15},
//    {66, 16}, {72, 18}, {80, 20}
//  }.
//

void    set_tx_power(int ppp)

{
    int calc = ppp * 4;

    if(calc > gl_maxpow)
        calc = gl_maxpow;

    esp_wifi_set_max_tx_power(calc);

    int8_t pow = 0;
    esp_wifi_get_max_tx_power(&pow);

    printf("%d Power (%d) on board is set to %d\n", get_ms(), calc, pow);

    // Measured: maximum of 78
    //for(int aa = 0; aa < 255; aa++)  {
    //    esp_wifi_set_max_tx_power(aa);
    //    printf("%d-%d   ", aa, pow);
    //    }
}

// -----------------------------------------------------------------------
// Read NVS into variables

void    read_vars()

{
    nvs_handle my_handle;

    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);

    if (err != ESP_OK)
        {
        printf("Error (%d) opening NVS handle.", err);
        return;
        }
    unsigned int olen = sizeof(gl_devname);
    err = nvs_get_str(my_handle, "iocomname", gl_devname, &olen);
    if (err != ESP_OK)
        {
        //if (verbose)
        //    ESP_LOGE(TAG, "Warn (%d) reading NVS agv", err);
        }
    //printf("NVS  name: '%s'\n", gl_devname);

    unsigned int olen2 = sizeof(gl_netname);
    err = nvs_get_str(my_handle, "netname", gl_netname, &olen2);
    if (err != ESP_OK)
        {
        //if (verbose)
        //    ESP_LOGE(TAG, "Warn (%d) reading NVS agv", err);
        }
    unsigned int olen3 = sizeof(gl_netpass);
    err = nvs_get_str(my_handle, "netpass", gl_netpass, &olen3);
    if (err != ESP_OK)
        {
        //if (verbose)
        //    ESP_LOGE(TAG, "Warn (%d) reading NVS agv", err);
        }
    //printf("NVS  name: '%s'\n", gl_devname);
    long int intval = 0;
    err = nvs_get_i32(my_handle, "pow",  &intval);
    if (err == ESP_OK)
        {
        gl_power = intval;
        if(gl_power == 20)
            {
            //printf("Not commiting at full power, allow default.\n");
            }
        else
            {
            //set_tx_power(gl_power);
            }
        }
    //printf("NVS  pow: '%d'\n", gl_power);

    err = nvs_get_i32(my_handle, "repcnt",  &intval);
    if (err == ESP_OK)
        {
        if(intval < 1)
            intval = 1;
        if(intval > 12)
            intval = 12;
        gl_repcnt = intval;
        }
    err = nvs_get_i32(my_handle, "rep",  &intval);
    if (err == ESP_OK)
        {
        if(intval < 200)
            intval = 200;
        if(intval > 700)
            intval = 700;
        gl_repeat = intval;
        }
    //printf("NVS  pow: '%d'\n", gl_power);

    nvs_close(my_handle);
}

// EOF
