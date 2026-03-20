
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
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_now.h"
//#include "esp_pm.h"
//#include "driver/gpio.h"
//#include "esp32/rom/ets_sys.h"
//#include "esp32/rom/crc.h"
//#include "driver/i2c.h"
#include "lwip/err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "utils.h"

static char *TAG= "IO4_utils";

//////////////////////////////////////////////////////////////////////////
//
// Parse list of names into buffers. List terminated with NULL.
// Must pass as many buffs as names.
// Warning: string must be unique - no sub strings as strstr will match it
//

int     parse_post(const char *buf, const char *names[], char *mems[], int xlen)

{
    int prog = 0, len;

    //ESP_LOGI(TAG, "Called parser with buf '%s'\n", buf);

    while(true)
        {
        if(names[prog] == NULL)
            break;
        if(mems[prog] == NULL)
            break;

        char *ppp = strstr(buf, names[prog]);
        mems[prog][0] = '\0';                   // Clear target
        if(ppp)
            {
            ppp += strlen(names[prog]);

            char *ppp2 = strstr(ppp, "&");
            if(ppp2)
                {
                len = MIN(ppp2 - ppp, xlen - 1);
                }
            else
                {
                // Last one
                len = MIN(strlen(ppp), xlen - 1);
                }
            memcpy(mems[prog], ppp, len);
            // Zero terminate
            mems[prog][len] = '\0';
            }
        prog++;
        //FEED_DOG
        }

    //for(int loopd = 0; loopd < prog; loopd += 1)
    //    ESP_LOGI(TAG, "name='%s' val='%s'\n", names[loopd], mems[loopd]);

    return prog;
}

//////////////////////////////////////////////////////////////////////////
// Print parse results

int     print_posts(const char *names[], char *mems[])

{
   for(int aa = 0; names[aa] != NULL; aa++)
        {
        printf("%s'%s'  ", names[aa], mems[aa]);
        }
    printf("\n");

    return 0;
}

//////////////////////////////////////////////////////////////////////////

char    *xstrdup(const char *str)

{
    int tlen = strlen(str);
    char *mem = malloc(tlen + 2);
    if(mem)
        memcpy(mem, str, tlen + 1);
    return(mem);
}

char    *xsnprintf(char *format, ...)

{
    va_list ap, ap2;
    printf("'%s'\n", format);
    va_start(ap, format);
    va_copy(ap2, ap);
    //printf(";%d;\n", va_arg(ap, int));
    int len = vsnprintf(NULL, 0, format, ap);
    va_end(ap);

    if(len < 0)
        return NULL;
    char *ptr = malloc(len + 20);
    if(ptr == NULL)
        return NULL;
    len = vsnprintf(ptr, len, format, ap2);
    va_end(ap2);
    if(len < 0)
        {
        free(ptr);
        return NULL;
        }
    //printf("ptr: %d '%s'\n", len, ptr);
    return ptr;
}

//////////////////////////////////////////////////////////////////////////
// Substitute string substr in orgx replace it with restr (result string)
// String is padded and limited to the length of substx.
//

char    *subst_str(char *orgx, const char *substx, const char *restr)

{
    int lenx = strlen(substx);
    char *ggg = strstr(orgx, substx), *tmpstr = malloc(lenx + 4);

    if(tmpstr == NULL)
        {
        ESP_LOGE(TAG, "subst_str() no memory.");
        goto endd2;
        }
    // Normalize string to lenx / lenx
    if(ggg)
        {
        snprintf(tmpstr, lenx + 2, "%-*.*s", lenx, lenx, restr);
        memcpy(ggg, tmpstr, strlen(tmpstr));
        }
    else
        {
        // Only display on request
        //if(debug_strlib)
        //    ESP_LOGE(TAG, "subst_str() cannot find anchor: %s", substx);
        }
    free(tmpstr);

    endd2:
    return ggg;
}

//////////////////////////////////////////////////////////////////////////
// Substitute string substr in orgx after atstr, and replace it
// with restr (result string)
// String is padded and limited to the length of substx.
//

char    *subst_str_at(char *orgx, cchar *atstr, cchar *substx, cchar *restr)

{
    char *retp = NULL;

    char *anchor = strstr(orgx, atstr);
    if(anchor)
        {
        retp = subst_str(anchor, substx, restr);
        }
    else
        {
        //if(debug_strlib)
        //    ESP_LOGE(TAG, "No atstr '%s' at subst_str_at.", atstr);
        }
    return retp;
}

//////////////////////////////////////////////////////////////////////////

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

void    cut_tailspace(char* str)

{
    char *p, *q;

    // Go to end
    for (p = q = str; *p != 0; p++)
        {
        }
    // scan until non space
    for (--p; p >= str; p--)
        {
        if (*p == ' ')
            *p = '\0';
        else
            break;
        }
}

#include "freertos/FreeRTOS.h"

#define NOP() asm volatile ("nop")

unsigned long IRAM_ATTR micros()
{
    return (unsigned long) (esp_timer_get_time());
}

void IRAM_ATTR delayMicroseconds(uint32_t us)
{
    uint32_t m = micros();
    if(us)
        {
        uint32_t e = (m + us);
        if(m > e)
            { //overflow
            while(micros() > e)
                {
                NOP();
                }
            }
        while(micros() < e)
            {
            NOP();
            }
        }
}

// point to after the '?'

char    *get_arg_ptr(const char *keys)
{
    int aa = 0; char *ret = NULL;
    while(1)
        {
        char chh = keys[aa];
        if (chh == '\0')
            break;
        if (chh == '?')
            break;
        aa++;
        }
    if (*keys == '\0')
        {
        printf("No Key in url");
        return NULL;
        }
    printf("keys: '%s'\n", &keys[11]);
    retrurn &keys[aa];
}

//////////////////////////////////////////////////////////////////////////
// Unescape HTML entities

int     unescape_url(char *str, char *strout, int lims)

{
    int loop, cnt = 0, len = strlen(str);

    for(loop = 0; loop < len; loop++)
        {
        if(str[loop] == '%')
            {
            if(loop + 2 < len)
                {
                if(isxdigit((int)str[loop+1]) && isxdigit((int)str[loop+2]) )
                    {
                    char chh_arr[3];
                    chh_arr[0] = str[loop+1]; chh_arr[1] = str[loop+2];
                    chh_arr[2] = '\0';
                    int val;
                    sscanf(chh_arr, "%x", &val);

                    if(cnt >= lims)
                        break;

                    strout[cnt++] = (char)(val & 0xff);
                    loop += 2;
                    }
                else
                    {
                    if(cnt >= lims)
                        break;
                    strout[cnt++] = str[loop];
                    }
                }
            else
                {
                if(cnt >= lims)
                    break;
                strout[cnt++] = str[loop];
                }
            }
        else if(str[loop] == '+')
            {
            strout[cnt++] = ' ';
            }
        else
            {
            if(cnt >= lims)
                break;
            strout[cnt++] = str[loop];
            }
        }
    strout[cnt++] = '\0';
    return cnt;
}

// The stdlib is borken ....

double  atofx(const unsigned char *s)

{
    unsigned long int ii = 0;

    for(ii = 0; isspace(s[ii]); ++ii)
        ;  /*skip white space*/

    int sign;
    sign = (s[ii] == '-')? -1 : 1; /*The sign of the number*/

    if(s[ii] == '-' || s[ii] == '+'){
        ++ii;
    }

    double value;
    for(value = 0.0; isdigit(s[ii]); ++ii){
        value = value * 10.0 + (s[ii] - '0');
    }

    if(s[ii] == '.'){
        ++ii;
    }

    double power;
    for(power = 1.0; isdigit(s[ii]); ++ii){
        value = value * 10.0 + (s[ii] - '0');
        power *= 10.0;
    }

    if(s[ii] == 'e' || s[ii] == 'E'){
        ++ii;
    }
    else{
        return sign * value/power;
    }

    int powersign; /*The sign following the E*/
    powersign = (s[ii] == '-')? -1 : 1;

    if(s[ii] == '-' || s[ii] == '+'){
        ++ii;
    }

    int power2; /*The number following the E*/
    for(power2 = 0; isdigit(s[ii]); ++ii){
        power2 = power2 * 10 + (s[ii] - '0');
    }

    if(powersign == -1){
        while(power2 != 0){
            power *= 10;
            --power2;
        }
    }
    else{
        while(power2 != 0){
            power /= 10;
            --power2;
        }
    }

    return sign * value/power;
}

int     enc_str(char *str, int len, char *key, int klen, char *out, int olen)

{
    int kprog = 0, prog = 0, oprog = 0;
    int padded = len + len % 2;

    //printf("str=%s, len=%d key=%s, klen=%d\n", str, len, key, klen);

    // Copy string
    char *mem = malloc(padded+2);
    if(!mem)
        return -1;
    memset(mem, '\0', padded);
    memcpy(mem, str, len);
    for(int aa = 0; aa < padded; aa++)
        {
        mem[prog] ^= key[kprog];
        prog++; kprog++;
        if(kprog >= klen)
            kprog = 0;
        }
    for(int aa = 0; aa < len; aa++)
        {
        if(oprog >= olen-2)
            break;
        snprintf(out + oprog, 3, "%02x", mem[aa]);
        oprog += 2;
        }
    free(mem);

    out[oprog] = '\0';
    return oprog;
}

int     dec_str(char *str, int len, char *key, int klen, char *out, int olen)

{
    int kprog = 0, prog = 0, zprog = 0, oprog = 0, dprog = 0;
    int padded = len + len % 2;

    //printf("str=%s, len=%d key=%s, klen=%d\n", str, len, key, klen);
    // Copy string
    char *mem = malloc(padded+2);
    if(!mem)
        return -1;
    memset(mem, '\0', padded);

    for(int aa = 0; aa < len/2; aa++)
        {
        unsigned int chh;
        sscanf(str + prog, "%02x", &chh);
        mem[zprog] = (char)chh;
        prog += 2; zprog++;
        }
    mem[len] = '\0';

    for(int aa = 0; aa < padded/2; aa++)
        {
        mem[dprog] ^= key[kprog];
        dprog++; kprog++;
        if(kprog >= klen)
            kprog = 0;
        }
    for(int aa = 0; aa < len/2; aa++)
        {
        if(oprog >= olen-2)
            break;
        out[oprog] = mem[oprog];
        oprog += 1;
        }
    free(mem);
    out[oprog] = '\0';

    return len/2;
}

//////////////////////////////////////////////////////////////////////////// EOF
// Convert current clock to string
// Fri 17/04/20 14:16:12  shortened time stamp str

char    *time2str(int64_t ttt)

{
    static char  tmp[24];

    int tt = (int)(ttt / 1000);
    int t2 = (int)(tt / 1000);

    //int hh =  (t / (60 * 60)) % 60;
    int mm =  (t2 / 60) % 60;
    int ss =  (t2  % 60);
    int ff =  (tt  % 1000);

    //snprintf(tmp, sizeof(tmp), "%02d:%02d:%02d.%02d", hh, mm, ss, ff);
    snprintf(tmp, sizeof(tmp), "%02d:%02d.%03d", mm, ss, ff);
    return tmp;
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

// EOF
