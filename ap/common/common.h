
/* =====[ access point template project ]=================================

   File Name:       common.c

   Description:

   Revisions:

      REV       DATE               BY          DESCRIPTION
      ----  -----------         ----------      -------------------------
      0.00  Tue 10.Mar.2026     Peter Glen      Initial version.

   ======================================================================= */

#define LED1        GPIO_NUM_19
#define LED2        GPIO_NUM_18
#define LED3        GPIO_NUM_5
#define LED4        GPIO_NUM_23

#define INPUT1      GPIO_NUM_25
#define INPUT2      GPIO_NUM_26
#define INPUT3      GPIO_NUM_27

// This is the original on the IOCOM board #1
#define INPUT4      GPIO_NUM_33

// This is a fix for the wrong port on PCB
//#define INPUT4      GPIO_NUM_32

#define RELAY1      GPIO_NUM_2
#define RELAY2      GPIO_NUM_13
#define RELAY3      GPIO_NUM_16
#define RELAY4      GPIO_NUM_17

#define CONF_BUTT   GPIO_NUM_4
#define BATT_A2D    GPIO_NUM_36

#define NUM_LEDS        4
#define NUM_BUTTONS     4

#define NUM_MODES       11                  // How many modes are in this one
#define IOCOM_VERSION   "1.0"
#define IOCOM_DATE      "Thu 29.Apr.2021"
#define IOCOM_ALIVE     3

#define VDIV            20                      // Virtual volt divisor
#define V2DV(aa)        (int)((aa) * VDIV)      // Volt to fractional volt
#define DV2V(aa)        ((float)(aa) / VDIV)    // Fractional volt to vold

#define BATT_ELBOW      64                  // fractional volts (div by 20) 64 -> 3.2 volts

// -----------------------------------------------------------------------
//
// time must be divisable by 20
//

typedef struct _led_script
{
    int  led_ontime;
    int  led_cycletime;
    int  led_cyclecnt;      // Set to -1 fo forever

    // Internally used
    int  _led_mark;         // Used by the script
    int  _old_state;
} Led_Script;

#define LED_ON      0
#define LED_OFF     1

extern Led_Script   ls1, ls2, ls3, ls4;

// ------------------------------------------------------------------
// Macro to init ESP32 port to defaults
//  defx = GPIO_PORT_NUM
//  dir = GPIO_MODE_INPUT ... or ... GPIO_MODE_OUTPUT
//  Declare / use:
//        gpio_config_t gpioConfig;
//        DECL_INIT_IO(GPIO_NUM_XX, GPIO_MODE_INPUT);
//        DECL_INIT_IO(GPIO_NUM_YY, GPIO_MODE_OUTPUT);

#define DECL_INIT_IO(defx, dir)                           \
      gpioConfig.pin_bit_mask = 1ULL;                     \
      gpioConfig.pin_bit_mask <<= defx;                   \
      gpioConfig.mode = dir;                              \
      gpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;        \
      gpioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;    \
      gpioConfig.intr_type = GPIO_INTR_DISABLE;           \
      ESP_ERROR_CHECK(gpio_config(&gpioConfig));          \

extern  int         gl_power;
extern  int         gl_repeat;
extern  int         gl_repcnt;

extern  uint64_t    gl_last_http;

extern  char        gl_key[];
extern  uint8_t     portarr[4];
extern  uint8_t     ledarr[4];

extern  uint64_t    gl_presscnt;
extern  int         gl_iam_battery;
extern  int         gl_webon;

extern  uint64_t    gl_light_milli;
extern  uint64_t    gl_deep_milli;
extern  char        gl_devname[24];
extern  int8_t      gl_maxpow;
extern  int         verbose;

extern  int         gl_stop_pair;
extern  int         gl_stop_listen;

extern  int         gl_mode_arr[NUM_BUTTONS / 2];

extern  int         gl_tout_1;
extern  int         gl_tout_2;
extern  int         gl_tout_3;
extern  int         gl_tout_4;

extern  int         gl_retrig[NUM_BUTTONS];
extern  int         gl_delay[NUM_BUTTONS];

void    dump_str2(const void *vptr, int len);
void    dump_str(const void *vptr, int len, char *out, int olen);
//void    preprocess_string(char* str);
int     get_ms();

void    init_gpios();
void    init_leds();

void    init_rtc_in(int gpio);
void    print_chipinfo();
void    print_wake_cause(int waker, char *context);
int     encrdecr(int mode, char *mem, int mlen, char *keyx, int klen);
int     encrdecrx(int mode, void *mem, int mlen, char *keyx, int klen);
int     inc_bootcount();
void    set_tx_power(int ppp);
void    read_vars();
void    delayed_reboot(int wait_ms);
int     get_butt_masks();

// EOF


