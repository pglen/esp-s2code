
/* =====[ loratest.sess ]========================================================

   File Name:       packets.c

   Description:     Functions for packets.c

   Revisions:

      REV   DATE                BY              DESCRIPTION
      ----  -----------         ----------      --------------------------
      0.00  Wed 25.Mar.2026     Peter Glen      Initial version.

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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"

#include "cJSON.h"

#include "utils.h"
#include "comline.h"
#include "protocol.h"
#include "packets.h"
#include "nvs.h"

#include "lora.h"

static const char *TAG = "comline";

#define VT_SAVECURSOR            "\e7"  /* Save cursor and attrib */
#define VT_RESTORECURSOR         "\e8"  /* Restore cursor pos and attribs */
#define VT_SAVECURSOR2           "\es"  /* Save cursor and attrib */
#define VT_RESTORECURSOR2        "\eu"  /* Restore cursor pos and attribs */
#define VT_SETWIN_CLEAR          "\e[r" /* Clear scrollable window size */
#define VT_CLEAR_SCREEN          "\e[2J" /* Clear screen */
#define VT_CLEAR_LINE            "\e[2K" /* Clear this whole line */
#define VT_RESET_TERMINAL        "\ec"

void    out_str(const char *strx, const char *str2)

{
    printf(VT_SAVECURSOR);
    printf("\033[1;12r");
    printf("\033[12;1H");
    printf("%s", strx);
    printf("%s", str2);
    //printf("\033[1;24r");
    printf("\033[13;24r");
    //printf("\033[18;1H");
    printf(VT_RESTORECURSOR);
}

int splity = 12;

void    set_split()
{
    printf("\033[%d;24r", splity);
}


typedef struct {
    struct arg_str *arg1;
    struct arg_end *end;
} arg_args_t;

typedef struct {
    struct arg_int *arg1;
    struct arg_end *end;
} arg_argf_t;

static arg_args_t  spread_args;

//int spread = 12;

static int set_spread(int argc, char **argv)

{
    int nerrors = arg_parse(argc, argv, (void **) &spread_args);
    if (nerrors != 0) {
    }
    if(strlen(spread_args.arg1->sval[0]) == 0)
        {
        printf("Current spread factor: %s\n", gl_spread);
        return 0;
        }
    if(spread_args.arg1->sval[0][0] == '?')
        {
        printf("Spread factor: 6 - 12. Default: 10\n");
        return 0;
        }
    int sss = atoi(spread_args.arg1->sval[0]);
    if(sss > 12 || sss < 6)
        {
        printf("Invalid spread factor. Kept old: %s\n", gl_spread);
        return 0;
        }
    TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
    lora_set_spreading_factor(sss);
    GIVE_SEMA(sSemaphore);

    strncpy(gl_spread, spread_args.arg1->sval[0], sizeof(gl_spread));
    submit_nvs_str("spread",    gl_spread);
    printf("Spread set to %s\n", gl_spread);
    spread_args.arg1->sval[0] = "";
    return 0;
}

//10.4E3 //15.6E3 //20.8E3 //31.25E3//41.7E3 //62.5E3 //125E3
//250E3 //512E3

static arg_args_t  bw_args;

static int set_bw(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &bw_args);
    if (nerrors != 0) {
    }
    if(strlen(bw_args.arg1->sval[0]) == 0)
        {
        printf("Current bandwidth: %s\n", gl_bwidth);
        return 0;
        }
    if(bw_args.arg1->sval[0][0] == '?')
        {
        printf(
        "Bandwidth list: 10.4E3  15.6E3 20.8E3 31.25E3 41.7E3\n"
        "                 62.5E3 125E3  250E3 512E3\n");
        return 0;
        }
    float sss = atof(bw_args.arg1->sval[0]);
    if(sss < 1000)
        sss *= 1000;
    if (sss > 500E3 || sss < 5E3)
        {
        printf("Invalid bandwidth. Kept old: %s\n", gl_bwidth);
        return 0;
        }
    snprintf(gl_bwidth, sizeof(gl_bwidth), "%e", sss);
    TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
    lora_set_bandwidth(atof(gl_bwidth));
    GIVE_SEMA(sSemaphore);

    submit_nvs_str("bwidth",    gl_bwidth);

    printf("Bandwith set to %s\n", gl_bwidth);
    bw_args.arg1->sval[0] = "";
    return 0;
}

static arg_args_t  tr_args;

static int set_tr(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &tr_args);
    if (nerrors != 0) {
    }
    if(strlen(tr_args.arg1->sval[0]) == 0)
        {
        printf("Current trench: %s\n", gl_curr_tr);
        return 0;
        }
    if(tr_args.arg1->sval[0][0] == '?')
        {
        printf("Set current trench");
        return 0;
        }
    int ttt = atoi(tr_args.arg1->sval[0]);
    if (ttt > 0xffff || ttt < 0x0)
        {
        printf("Invalid trench. Kept old: %s\n", gl_curr_tr);
        return 0;
        }
    snprintf(gl_curr_tr, sizeof(gl_curr_tr), "%d", ttt);
    printf("Trench set to %s\n", gl_curr_tr);
    tr_args.arg1->sval[0] = "";
    return 0;
}

static arg_args_t  pw_args;

static int set_pw(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &pw_args);
    if (nerrors != 0) {
    }
    if(strlen(pw_args.arg1->sval[0]) == 0)
        {
        printf("Current power: %s\n", gl_txpower);
        return 0;
        }
    if(pw_args.arg1->sval[0][0] == '?')
        {
        printf("Set power level 2 ... 15\n");
        return 0;
        }
    int ppp = atoi(pw_args.arg1->sval[0]);
    if (ppp > 15 || ppp < 2)
        {
        printf("Invalid power level. Kept old: %s\n", gl_txpower);
        return 0;
        }
    TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
    lora_set_tx_power(ppp);
    GIVE_SEMA(sSemaphore);

    strncpy(gl_txpower, pw_args.arg1->sval[0], sizeof(gl_txpower));
    submit_nvs_str("txpower",   gl_txpower);
    printf("Power set to %s\n", gl_txpower);
    pw_args.arg1->sval[0] = "";
    return 0;
}

static arg_args_t  fr_args;

static int set_fr(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &bw_args);
    if (nerrors != 0) {
    }
    if(strlen(bw_args.arg1->sval[0]) == 0)
        {
        printf("Current frequency: %s\n", gl_txfreq);
        return 0;
        }
    if(bw_args.arg1->sval[0][0] == '?')
        {
        printf("Frequency: 410 - 530 Mhz (def: 433.375E6)\n");
        printf("ch1=433.175E6 ch2=433.425E6 ...\n");
        printf("Frequency is capped within legal limits.\n");
        return 0;
        }
    float sss = atof(bw_args.arg1->sval[0]);
    // Correct it
    if(sss < 1000000)
        sss *= 1000000;
    if (sss > 530E6 || sss < 410E6)
        {
        printf("Invalid frequency. Kept old: %s\n", gl_txfreq);
        return 0;
        }
    snprintf(gl_txfreq, sizeof(gl_txfreq), "%e", sss);
    printf("gl_txfreq: %s\n", gl_txfreq);

    TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
    lora_set_frequency(sss);
    GIVE_SEMA(sSemaphore);
    printf("Frequency set to %f\n", sss);
    submit_nvs_str("txfreq",  gl_txfreq);

    bw_args.arg1->sval[0] = "";
    return 0;
}

static arg_args_t  tu_args;

static int set_tu(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &tu_args);
    if (nerrors != 0) {
    }
    if(strlen(tu_args.arg1->sval[0]) == 0)
        {
        printf("Current adjusted frequency: %s\n", gl_txfreq);
        return 0;
        }
    if(tu_args.arg1->sval[0][0] == '?')
        {
        printf("Adjust Frequency. Use +- diff\n");
        return 0;
        }
    double sss = atofx(gl_txfreq);
    int ttt = atoi(tu_args.arg1->sval[0]);
    sss += (double)ttt;
    printf("Adjustment %d %f\n", ttt, sss);
    snprintf(gl_txfreq, sizeof(gl_txfreq), "%e", sss);
    printf("gl_txfreq: %s\n", gl_txfreq);

    TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
    lora_set_frequency(sss);
    GIVE_SEMA(sSemaphore);
    printf("Frequency adjusted to %f\n", sss);
    submit_nvs_str("txfreq",  gl_txfreq);

    tu_args.arg1->sval[0] = "";
    return 0;
}

static arg_args_t  td_args;

static int set_td(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &tu_args);
    if (nerrors != 0) {
    }
    if(strlen(tu_args.arg1->sval[0]) == 0)
        {
        printf("Current adjusted frequency: %s\n", gl_txfreq);
        return 0;
        }
    if(tu_args.arg1->sval[0][0] == '?')
        {
        printf("Adjust Frequency. Use +- diff\n");
        return 0;
        }
    double sss = atofx(gl_txfreq);
    int ttt = atoi(tu_args.arg1->sval[0]);
    sss -= (double)ttt;
    printf("Adjustment %d %f\n", ttt, sss);
    snprintf(gl_txfreq, sizeof(gl_txfreq), "%e", sss);
    printf("gl_txfreq: %s\n", gl_txfreq);

    TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
    lora_set_frequency(sss);
    GIVE_SEMA(sSemaphore);
    printf("Frequency adjusted to %f\n", sss);
    submit_nvs_str("txfreq",  gl_txfreq);

    tu_args.arg1->sval[0] = "";
    return 0;
}

static arg_args_t  str_trargs;

static int set_trstr(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &str_trargs);
    if (nerrors != 0) {
    }
    if(strlen(str_trargs.arg1->sval[0]) == 0)
        {
        printf("Cannot send empty string\n");
        return 0;
        }
    if(str_trargs.arg1->sval[0][0] == '?')
        {
        printf("Pass string");
        return 0;
        }
    send_payload(str_trargs.arg1->sval[0], atoi(gl_curr_tr));
    str_trargs.arg1->sval[0] = "";
    return 0;
}

int verbose  = 0;
static arg_args_t  verb_args;
static int set_verbose(int argc, char **argv)

{
    //printf("verb_args called\n");
    int nerrors = arg_parse(argc, argv, (void **) &verb_args);

    if (nerrors != 0) {
        //arg_print_errors(stderr, verb_args.end, argv[0]);
        }
    if(strlen(verb_args.arg1->sval[0]) == 0)
        {
        printf("Current verbosity level: %d\n", verbose);
        return 0;
        }
    //printf("arg '%s'\n", verb_args.verb->sval[0]);
    int vvv = atoi(verb_args.arg1->sval[0]);
    if(vvv <  0)
        vvv = 0;
    if(vvv > 10)
        vvv = 10;
    verbose = vvv;
    printf("Verbose set to %d\n", verbose);
    verb_args.arg1->sval[0] = "";

    return 0;
}

static arg_args_t  de_args;

static  int set_de(int argc, char **argv)
{

    int nerrors = arg_parse(argc, argv, (void **) &de_args);
    if (nerrors != 0) {
    }
    if(de_args.arg1->sval[0][0] == '?')
        {
        printf("Set LORA parameter to defaults\n");
        return 0;
        }
    printf("Setting LORA defaults. Are you sure? y/N ... ");  fflush(stdout);
    int ccc = getchar();
    if(ccc != 'y' && ccc != 'Y')
        {
        printf("\nSetting defaults aborted on user request.\n");
        return 0;
        }
    printf("\n");
    strncpy(gl_spread,  DEF_SPREAD,  sizeof(gl_spread));
    strncpy(gl_bwidth,  DEF_BWIDTH,  sizeof(gl_bwidth));
    strncpy(gl_txpower, DEF_POWER,   sizeof(gl_txpower));
    strncpy(gl_txfreq,  DEF_FREQ,    sizeof(gl_txfreq));
    //strncpy(gl_deftren, DEF_TRENCH,  sizeof(gl_deftren));

    TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
    lora_set_frequency(atof(gl_txfreq));
    lora_set_tx_power(atof(gl_txpower));
    lora_set_spreading_factor(atof(gl_spread));
    lora_set_bandwidth(atof(gl_bwidth));
    GIVE_SEMA(sSemaphore);

    submit_nvs_str("spread",    gl_spread);
    submit_nvs_str("bwidth",    gl_bwidth);
    submit_nvs_str("txpower",   gl_txpower);
    submit_nvs_str("txfreq",    gl_txfreq);

    //submit_nvs_str("deftrench", gl_deftren);
    printf("Defaults set. [fr=%s bw=%s sf=%s pw=%s]\n",
                        DEF_FREQ, DEF_BWIDTH, DEF_SPREAD, DEF_POWER);
    de_args.arg1->sval[0] = "";
    return 0;
}

static arg_args_t  pr_args;

static  int set_pr(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &pr_args);
    if (nerrors != 0) {
    }
    if(pr_args.arg1->sval[0][0] == '?')
        {
        printf("Show current and default parameters.\n");
        return 0;
        }
    printf("Defaults: [fr=%s bw=%s sf=%s pw=%s]\n",
                                    DEF_FREQ, DEF_BWIDTH, DEF_SPREAD, DEF_POWER);
    printf("Current:  [fr=%s bw=%s sf=%s pw=%s]\n",
                                    gl_txfreq, gl_bwidth, gl_spread, gl_txpower);
    pr_args.arg1->sval[0] = "";
    return 0;
}

static  int reboot_dev(int argc, char **argv)

{
    //printf("\033[18;24r");
    printf("Rebooting ..... \n");
    //vTaskDelay((int)parm / portTICK_PERIOD_MS);
    esp_restart();
    return 0;
}

static  int help(int argc, char **argv)

{
    printf("Commands: v (verbose) [1-10]; h (help); r (reboot);\n");
    printf("          w (pw) power level [2-15]\n");
    printf("          s (sf) spread [6-12]\n");
    printf("          b (bw) bandwidth [5-500]\n");
    printf("          f (fr) frequency [410.0-530.0] (Clamped to legal limits)\n");
    printf("          u (tu) tune frequency up  by Hz\n");
    printf("          n (td) tune frequency down by Hz \n");
    printf("          d (de) defaults [fr=%s bw=%s sf=%s pw=%s] \n",
                                    DEF_FREQ, DEF_BWIDTH, DEF_SPREAD, DEF_POWER);
    printf("          t (tr) transmit string \n");
    printf("          r (re) set trench number \n");
    printf("          p (pr) print current and default configuration\n");
    printf("Use: 'command ?' for help on a particular command.\n");
    return 0;
}
                                                            \
#define INIT_STRUCT(sss)                                    \
    sss.arg1 = arg_str1(NULL, NULL, "", "");                \
    sss.end  = arg_end(1);

#define INIT_STRUCTF(sss)                                    \
    sss.arg1 = arg_int1(NULL, NULL, "", "");                 \
    sss.end  = arg_end(1);

#define DECL_COMMANDx(namex, helpx, funcx, argx)            \
    {                                                       \
    const esp_console_cmd_t varx =  {                       \
        .command = namex,                                   \
        .help = helpx,                                      \
        .hint = NULL,                                       \
        .func = funcx,                                      \
        .argtable = argx,                                   \
        };                                                  \
    ESP_ERROR_CHECK( esp_console_cmd_register(& varx));     \
    }

void register_cmds(void)

{
    INIT_STRUCT(verb_args)
    DECL_COMMANDx("verb",       "Set verbosity", &set_verbose, &verb_args);
    DECL_COMMANDx("verbose",    "Set verbosity", &set_verbose, &verb_args);
    DECL_COMMANDx("v",          "Set verbosity", &set_verbose, &verb_args);

    DECL_COMMANDx("restart",    "reboot / restart device",  &reboot_dev, NULL);
    DECL_COMMANDx("reboot",     "reboot / restart device",  &reboot_dev, NULL);
    DECL_COMMANDx("r",          "reboot / restart device",  &reboot_dev, NULL);

    DECL_COMMANDx("help",       "Help",  &help, NULL);
    DECL_COMMANDx("h",          "Help",  &help, NULL);
    DECL_COMMANDx("?",          "Help",  &help, NULL);

    INIT_STRUCT(spread_args)
    DECL_COMMANDx("spread",     "Set spread", &set_spread, &spread_args);
    DECL_COMMANDx("sf",         "Set spread", &set_spread, &spread_args);
    DECL_COMMANDx("s",          "Set spread", &set_spread, &spread_args);

    INIT_STRUCT(bw_args)
    DECL_COMMANDx("bw",         "Set bandwidth", &set_bw, &bw_args);
    DECL_COMMANDx("b",          "Set bandwidth", &set_bw, &bw_args);

    INIT_STRUCT(tr_args)
    DECL_COMMANDx("re",         "Set trench", &set_tr, &tr_args);
    DECL_COMMANDx("r",          "", &set_tr, &tr_args);

    INIT_STRUCT(pw_args)
    DECL_COMMANDx("pw",         "Set pow", &set_pw, &pw_args);
    DECL_COMMANDx("w",          "", &set_pw, &pw_args);

    INIT_STRUCT(fr_args)
    DECL_COMMANDx("fr",         "Set frequency", &set_fr, &fr_args);
    DECL_COMMANDx("f",          "", &set_fr, &fr_args);

    INIT_STRUCT(tu_args)
    DECL_COMMANDx("tu",         "Tune frequency up", &set_tu, &tu_args);
    DECL_COMMANDx("u",          "", &set_tu, &tu_args);

    INIT_STRUCT(td_args)
    DECL_COMMANDx("td",         "Tune frequency down", &set_td, &td_args);
    DECL_COMMANDx("n",          "", &set_td, &td_args);

    INIT_STRUCT(de_args)
    DECL_COMMANDx("de",         "reset defaults", &set_de, &de_args);
    DECL_COMMANDx("d",          "", &set_de, &fr_args);

    INIT_STRUCT(pr_args)
    DECL_COMMANDx("pr",         "print config", &set_pr, &de_args);
    DECL_COMMANDx("p",          "", &set_pr, &pr_args);

    INIT_STRUCT(str_trargs)
    DECL_COMMANDx("t",         "transmit",  &set_trstr, &str_trargs);
    DECL_COMMANDx("tr",         "transmit", &set_trstr, &str_trargs);
}

void    start_console(char *prompt)

{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config =
                                ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t uart_config =
                                ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

    repl_config.prompt = prompt;

    // init console REPL environment
    ESP_ERROR_CHECK(esp_console_new_repl_uart(
                                &uart_config, &repl_config, &repl));
    //linenoiseSetDumbMode(1);
    linenoiseSetDumbMode(0);

    /* Register commands */
    register_cmds();

    vTaskDelay(pdMS_TO_TICKS(100));
    // start console REPL
    ESP_ERROR_CHECK(esp_console_start_repl(repl));

    //while(1)
    //    {
    //    int ret, retcode;
    //    char *line = linenoise("LoraWiFI> ");
    //    printf("Command: '%s'\n", line);
    //    ret = esp_console_run(line, &retcode);
    //    printf("ret=%d retcode=%d\n", ret, retcode);
    //    linenoiseFree(line);
    //    }
}

// EOF
