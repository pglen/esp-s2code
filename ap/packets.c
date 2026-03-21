
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

#define FALSE 0
#define TRUE 1

//static const char *TAG = "recv";

#define BLINK_GPIO CONFIG_BLINK_GPIO

static uint8_t s_led_state = 0;

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;

static void toggle_led(int ok)
{
    //printf("toggle\n");
    /* If the addressable LED is enabled */
    if (s_led_state) {
        if(ok)
            led_strip_set_pixel(led_strip, 0, 0, 36, 0);
        else
            led_strip_set_pixel(led_strip, 0, 36, 0, 0);

        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    } else {
        /* Set all LED off to clear all pixels */

        if(ok)
            led_strip_set_pixel(led_strip, 0, 0, 0, 0);
        else
            led_strip_set_pixel(led_strip, 0, 0, 0, 36);

        led_strip_refresh(led_strip);
        //led_strip_clear(led_strip);
    }
    s_led_state = ! s_led_state;
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Recv configured to blink addressable LED! %d", BLINK_GPIO);
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
}

#elif CONFIG_BLINK_LED_GPIO

static void toggle_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

# endif

#define VT_SAVECURSOR            "\e7"  /* Save cursor and attrib */
#define VT_RESTORECURSOR         "\e8"  /* Restore cursor pos and attribs */
#define VT_SAVECURSOR2           "\es"  /* Save cursor and attrib */
#define VT_RESTORECURSOR2        "\eu"  /* Restore cursor pos and attribs */
#define VT_SETWIN_CLEAR          "\e[r" /* Clear scrollable window size */
#define VT_CLEAR_SCREEN          "\e[2J" /* Clear screen */
#define VT_CLEAR_LINE            "\e[2K" /* Clear this whole line */
#define VT_RESET_TERMINAL        "\ec"

int splity = 12;

void  set_split()
{
    printf("\033[%d;24r", splity);
}

void out_str(const char *strx, const char *str2)

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

typedef struct {
    struct arg_str *arg1;
    struct arg_end *end;
} arg_args_t;

static arg_args_t  spread_args;

int spread = 12;

static int set_spread(int argc, char **argv)

{
    int nerrors = arg_parse(argc, argv, (void **) &spread_args);
    if (nerrors != 0) {
    }
    if(strlen(spread_args.arg1->sval[0]) == 0)
        {
        printf("Current spread level: %d\n", spread);
        return 0;
        }
    int sss = atoi(spread_args.arg1->sval[0]);
    spread = sss;

    lora_set_spreading_factor(spread);

    printf("Spread set to %d\n", spread);
    spread_args.arg1->sval[0] = "";
    return 0;
}

float gl_bw = 31.25E3;
static arg_args_t  bw_args;

//10.4E3 //15.6E3 //20.8E3 //31.25E3//41.7E3 //62.5E3 //125E3
//250E3 //512E3

static int set_bw(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &bw_args);
    if (nerrors != 0) {
    }
    if(strlen(bw_args.arg1->sval[0]) == 0)
        {
        printf("Current bandwidth: %.2f\n", gl_bw);
        return 0;
        }
    if(bw_args.arg1->sval[0][0] == '?')
        {
        printf(
        "Bandwidth list: 20.8E3 31.25E3 41.7E3 62.5E3 125E3 250E3 512E3\n");
        return 0;
        }
    float sss = atof(bw_args.arg1->sval[0]);
    gl_bw = (int)sss;
    lora_set_bandwidth(gl_bw);
    printf("Bandwith set to %.2f\n", gl_bw);
    bw_args.arg1->sval[0] = "";
    return 0;
}

float gl_fr = 433.175E6;
static arg_args_t  fr_args;

static int set_fr(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &bw_args);
    if (nerrors != 0) {
    }
    if(strlen(bw_args.arg1->sval[0]) == 0)
        {
        printf("Current frequency: %f\n", gl_fr);
        return 0;
        }
    if(bw_args.arg1->sval[0][0] == '?')
        {
        printf("Frequency: 410 - 530 Mhz (433.175E6)\n");
        printf("ch1=433.175E6 ch2=433.425E6 ...\n");
        printf("Make sure frequency is within legal limits.\n");
        return 0;
        }
    float sss = atof(bw_args.arg1->sval[0]);
    if (sss > 530E6 || sss < 410E6)
        {
        printf("Invalid frequency. Kept old: %f\n", gl_fr);
        return 0;
        }
    gl_fr = (int)sss;
    lora_set_bandwidth(gl_fr);
    printf("Frequency set to %f\n", gl_fr);
    bw_args.arg1->sval[0] = "";

    return 0;
}

char    payload[64];

static arg_args_t  str_args;

static int set_str(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &bw_args);
    if (nerrors != 0) {
    }
    if(strlen(bw_args.arg1->sval[0]) == 0)
        {
        printf("Current str: %s\n", payload);
        return 0;
        }
    if(bw_args.arg1->sval[0][0] == '?')
        {
        printf("Pass string");
        return 0;
        }
    strncpy(payload, bw_args.arg1->sval[0], sizeof(payload));
    //gl_str = strdup(bw_args.arg1->sval[0]);
    printf("Payload: %s\n", payload);
    bw_args.arg1->sval[0] = "";

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
    printf("Setting defaults\n");
    return 0;
}

static  int reboot_dev(int argc, char **argv)

{
    printf("\033[18;24r");
    printf("Rebooting ..... \n");
    //vTaskDelay((int)parm / portTICK_PERIOD_MS);
    esp_restart();
    return 0;
}

static  int help(int argc, char **argv)

{
    printf("Commands: v (verbose) [1-10] h (help) r (reboot) s (sf) (spread) [2-12]\n");
    printf("          b (bw) bandwidth [1-10] f (fr) frequency [410-500]\n");
    printf("          d (de) defaults [f=433.175MHz sf=12 bw=30k] \n");
    printf("          t (tr) transmit [hello ] \n");
    printf("use: command ? for help on command");
    return 0;
}
                                                            \
#define INIT_STRUCT(sss)                                    \
    sss.arg1 = arg_str1(NULL, NULL, "", "");                \
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

    INIT_STRUCT(fr_args)
    DECL_COMMANDx("fr",         "Set frequency", &set_fr, &fr_args);
    DECL_COMMANDx("f",          "", &set_fr, &fr_args);

    INIT_STRUCT(de_args)
    DECL_COMMANDx("de",         "reset defaults", &set_de, &de_args);
    DECL_COMMANDx("d",          "", &set_fr, &fr_args);

    INIT_STRUCT(str_args)
    DECL_COMMANDx("t",         "transmit", &set_str, &str_args);
    DECL_COMMANDx("tr",         "transmit", &set_str, &str_args);
}

// -----------------------------------------------------------------------

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

    linenoiseSetDumbMode(1);
    //linenoiseSetDumbMode(0);

    /* Register commands */
    //register_system();
    register_cmds();

    vTaskDelay(pdMS_TO_TICKS(100));
    // start console REPL
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

//# define LINUX_TEST
# define INCLUDE_DUMP

int isalnum2(char chh)
{
    if(chh >= '0' && chh <= '9')
        return TRUE;
    if(chh >= ':' && chh <= '@')
        return TRUE;
    if(chh >= 'a' && chh <= 'z')
        return TRUE;
    if(chh >= '[' && chh <= '`')
        return TRUE;
    if(chh >= 'A' && chh <= 'Z')
        return TRUE;
    if(chh >= ' ' && chh <= '/')
        return TRUE;
    if(chh >= '{' && chh <= '~')
        return TRUE;
    return FALSE;
}

void    sdump(const char *str, int len, char *out, int maxlen)
{
    int aa = 0, was = 0, prog = 0;
    for(aa = 0; aa < len; aa++)
        {
        if(prog >= maxlen)
            {
            out[maxlen] = '\0';
            printf("sdump overflow at: %d\n", prog);
            return;
            }
        char bb = str[aa] & 0xff;
        if(isalnum2(bb))
            {
            prog += sprintf(out+prog, "%c", bb);
            was = 1;
            }
        else
            {
            if (was)
                prog += sprintf(out+prog, " ");
            prog += sprintf(out+prog, "%02x ", bb);
            was = 0;
            }
        //if (aa % 16 == 15)
        //    {
        //    prog += sprintf(out+prog, "\n");
        //    }
        }
    if (aa % 16 != 15)
        prog += sprintf(out+prog, "\n");
}

# ifdef INCLUDE_DUMP

void    hexdump(const char *str, int len, char *out, int maxlen)
{
    int aa = 0, sss = 0, prog = 0;
    for(aa = 0; aa < len; aa++)
        {
        if(prog >= maxlen)
            {
            out[maxlen] = '\0';
            printf("hexdump overflow at: %d\n", prog);
            return;
            }
        //if (aa % 16 == 0)
        //    prog += sprintf(out+prog, "0x%-3.2x- ", aa);
        prog += sprintf(out+prog, "%02x ", str[aa] & 0xff);
        if (aa % 16 == 15)
            {
            prog += sprintf(out+prog, " -  ");
            for(int bb = sss; bb < sss + 16; bb++)
                {
                if(prog >= maxlen)
                    {
                    out[maxlen] = '\0';
                    printf("hexdump overflow at: %d\n", prog);
                    return;
                    }
                if(isalnum2(str[bb]))
                    prog += sprintf(out+prog, "%c", str[bb]);
                else
                    prog += sprintf(out+prog, ".");
                }
            prog += sprintf(out+prog, "\n");
            sss = aa + 1;
            }
        }
    if (sss != aa )
        {
        for(int cc = aa; cc < aa + 16 - len % 16; cc++)
            {
            if(prog >= maxlen)
                {
                out[maxlen] = '\0';
                printf("hexdump overflow at: %d\n", prog);
                return;
                }

            prog += sprintf(out+prog, "   ");
            }
        prog += sprintf(out+prog, " -  ");
        for(int bb = sss; bb < sss + 16; bb++)
            {
            if(prog >= maxlen)
            {
            out[maxlen] = '\0';
            printf("hexdump overflow at: %d\n", prog);
            return;
            }

            if (isalnum2(str[bb]))
                prog += sprintf(out+prog, "%c", str[bb]);
            else
                prog += sprintf(out+prog, ".");
            }
        //sprintf(out, "sss %d aa, %d", sss, aa);
        }
    prog += sprintf(out+prog, "\n");
    *(out+prog) = '\0';
}

# endif

// Shuffle a string to 16 bit unique ID

int16_t lora_chksum(const char *str, int len)
{
    //printf("len %d str '%s'\n", len, str);
    uint16_t ret = 0;
    for(int aa = 0; aa < len; aa++)
        {
        uint16_t nn = (uint16_t)str[aa];
        uint16_t qq = nn << 7 | nn;
        ret += qq + 10000;
        ret ^= 0x5aa5;
        }
    //printf("sum ret %d\n", ret);
    return(ret);
}

typedef struct _Lora_Packet {
    uint16_t chk;         // checksum
    uint16_t tid;         // target ID
    uint16_t sid;         // sender ID
    uint8_t  ttl;         // time to live
    uint8_t  len;         // payload len
    uint8_t  *payload;    // payload here

} Lora_Packet;

// Store it in little endian (intel) order
// Note: ESP32 is big andian ...
// ... This is why we are not using structures

int     assemble_packet(Lora_Packet *ptr, const char *pay, char *outstr, int maxlen)
{
    int prog = 0;
    ptr->ttl = 4;
    uint16_t ccc = lora_chksum(pay, strlen(pay));

    // fill in struct

    outstr[prog] = ccc & 0xff; prog++;
    outstr[prog] = ccc >> 8; prog++;

    int lenoffs = prog;
    outstr[prog] = 0 & 0xff; prog++;

    int slen = snprintf(outstr + prog, maxlen - prog, "%s", pay);
    outstr[lenoffs] = slen & 0xff;
    return(prog += slen);
}

// Verify is valid packet

int     check_packet(const char *str, int len)

{
    uint16_t nsum = lora_chksum(str + 3, *(str+2) );
    uint16_t org = *(str) + (*(str+1) << 8);
    //printf("nsum %d org %d\n", nsum, org);
    return nsum == org;
}

// Common init

void init_lora_common()

{
    //lora_enable_crc(); // Enable CRC check

    lora_set_frequency(433.175e6); // Set the frequency
    lora_set_tx_power(15);
    lora_set_boost(1);
    lora_set_spreading_factor(12);
    lora_set_bandwidth(41.7E3);
    //lora_set_bandwidth(31.25E3);
    //lora_set_bandwidth(20.8E3);
}

#ifdef LINUX_TEST

char buff[64] = "";

#if 0
int main(int argv, char* argc[])

{
    srandom(time(NULL));
    int len = assemble_packet("hello 0123456789\n", buff, sizeof(buff));

    # ifdef  INCLUDE_DUMP
    hexdump(buff, len);
    # endif

    int ret = check_packet(buff, len);
    printf("Check: %d\n", ret);

    //char *strs;
    //printf("sum: %x\n", chksum(strs, strlen(strs)) & 0xffff);
    //strs = "12345678";
    //printf("sum: %x\n", chksum(strs, strlen(strs)) & 0xffff);
    //strs = "12345679";
    //printf("sum: %x\n", chksum(strs, strlen(strs)) & 0xffff);
    //strs = "22345679";
    //printf("sum: %x\n", chksum(strs, strlen(strs)) & 0xffff);

    return 0;
}

#endif

#endif
