/*
 * SPDX-FileCopyrightText: 2021-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "esp_console.h"

//include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "soc/adc_channel.h"

#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"

#include "../../console.c"

#define EXAMPLE_ADC_UNIT                    ADC_UNIT_1
#define EXAMPLE_ADC_CONV_MODE               ADC_CONV_SINGLE_UNIT_1
#define EXAMPLE_ADC_ATTEN                   ADC_ATTEN_DB_12
#define EXAMPLE_ADC_BIT_WIDTH               SOC_ADC_DIGI_MAX_BITWIDTH

esp_err_t ret;
uint32_t ret_num = 0;

static int adc_old = 0; //[2][10];
//static int adc_oldold = 0; //[2][10];
//static int adc_val = 0; //[2][10];
//static int adc_raw = 0; //[2][10];

#define BUFF_SIZE  1024
int16_t buff[BUFF_SIZE + 4] = {0,};
//int idx = 0;

//ADC1 Channels
#define EXAMPLE_ADC1_CHAN0          ADC_CHANNEL_1
#define EXAMPLE_ADC1_CHAN1          ADC_CHANNEL_2

#include "a2f.c"

int filt[OUTP_SIZE] = {0, };
int outp[OUTP_SIZE];

//static int voltage[2][10];
//static const char *TAG = "SND";
//static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);

uint8_t result[BUFF_SIZE] = {0};
adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};


#if 0
/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

#endif

void    init_adc1(adc_oneshot_unit_handle_t *adc1_handle)

{
    //-------------ADC1 Init---------------//
    //adc_oneshot_unit_handle_t adc1_handle;

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode =  ADC_ULP_MODE_DISABLE,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = EXAMPLE_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        //.bitwidth = ADC_BITWIDTH_12,
    };
    //ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle,
    //                                            EXAMPLE_ADC1_CHAN0, &config));

    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc1_handle,
                                                EXAMPLE_ADC1_CHAN1, &config));

    //-------------ADC1 Calibration Init---------------//
    //adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    //adc_cali_handle_t adc1_cali_chan1_handle = NULL;
    //bool do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN0, EXAMPLE_ADC_ATTEN, &adc1_cali_chan0_handle);
    //bool do_calibration1_chan1 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN1, EXAMPLE_ADC_ATTEN, &adc1_cali_chan1_handle);
}

uint64_t startp = 0;

void    loop_adc(adc_oneshot_unit_handle_t *adc1_handle)

{
    int idx2 = 0;
    while (1) {
        if(idx2 == 0)
            {
            #if 1
            startp = esp_timer_get_time();
            #endif
            }
        // 1  kHz -> 1000 micro sec per sample
        // 5  kHz -> 200 micro sec per sample
        // 10 kHz -> 100 micro sec per sample
        int adc_now;

        // Read more than required, divide for avarage
        //#define MULREAD 2           // 10 kilohertz
        //int sss = 0;
        //for(int ccc = 0; ccc < MULREAD; ccc++) {
        //    ESP_ERROR_CHECK(adc_oneshot_read(*adc1_handle,
        //                            EXAMPLE_ADC1_CHAN1, &adc_now));
        //    sss += adc_now;
        //    }
        //int adc_raw = sss / MULREAD;

        ESP_ERROR_CHECK(adc_oneshot_read(*adc1_handle,
                                    EXAMPLE_ADC1_CHAN1, &adc_now));
        int ddd = abs(adc_now - adc_old);
        if(ddd > 7000)
            {
            // Skip if diff is too large
            //adc_raw = adc_old;
            printf("print: skip %d old: %d ddd: %d\n", adc_now, adc_old, ddd);
            //adc_raw = adc_old;
            }
        else
            {
            buff[idx2] = adc_now - 0xec0;
            adc_old = adc_now;
            }
        if(idx2 >= BUFF_SIZE)
            {
            #if 0
            uint64_t end = esp_timer_get_time();
            printf("time per sample: %lld us total: %lld ms\n",
                        (end - startp)/BUFF_SIZE, (end - startp)/1000);
            #endif

            if (!stop_flag)
                {
                int64_t sum = 0;
                for( int aa = 0; aa < BUFF_SIZE; aa++)
                    {
                    sum += buff[aa];
                    printf("%5d ", buff[aa]);
                    if (aa % 12 == 11)
                        printf("\n");
                    }
                printf("\n");
                sum /= BUFF_SIZE;
                //printf("print:avg: %lld %llx\n", sum, sum & 0xffff);
                vTaskDelay(pdMS_TO_TICKS(10));
                }

            a2f(buff, BUFF_SIZE, outp, sizeof(outp));

            int damp = 2;
            for (int bb = 0; bb < sizeof(res)/sizeof(int); bb++)
                {
                filt[bb]  += (res[bb] - filt[bb]) / damp;
                }
            int avg = 0;
            for (int bb = 0; bb < sizeof(res)/sizeof(int); bb++)
                avg += filt[bb];
            avg /= sizeof(res)/sizeof(int);
            if (show_filt)
                {
                for (int bb = 0; bb < sizeof(res)/sizeof(int); bb++)
                    {
                    printf(" %+4d ", (int)(100 * (double)filt[bb]/avg));
                    }
                printf("\n");
                }
            vTaskDelay(pdMS_TO_TICKS(10));
            idx2 = 0;
            }
        idx2++;
        }
}

void task_snd(void *p) {

    //static TaskHandle_t s_task_handle;
    //s_task_handle = xTaskGetCurrentTaskHandle();
    //esp_task_wdt_delete(NULL);

    adc_oneshot_unit_handle_t adc1_handle;
    init_adc1(&adc1_handle);
    loop_adc(&adc1_handle);
    // not reached
}

void app_main(void)
{
    (void)startp;               // No warning
    xTaskCreate(&task_snd, "task_snd", 2048, NULL, 5, NULL);

    start_console("sndtest> ");

    //init_adc1();
    //printf("gpio %d\n", EXAMPLE_ADC1_CHAN1);
    while(1)
        vTaskDelay(pdMS_TO_TICKS(10));
}

//# EOF
