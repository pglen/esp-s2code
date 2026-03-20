
/* =====[ access point template project ]=================================

   File Name:       strlib.c

   Description:     string library for embedded

   Revisions:

      REV       DATE               BY          DESCRIPTION
      ----  -----------         ----------      -------------------------
      0.00  Tue 10.Mar.2026     Peter Glen      Initial version.

   ======================================================================= */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/param.h>

#ifndef LINUX_TEST

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
#include "lwip/err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#endif

#include "strlib.h"

//static char *TAG= "strlib";
static char *sent1 = "1234";
static char *sent2 = "5678";
static char *xlist[12] = {NULL, };

xStr    *xstr_create(int len)

{
    xStr *sss = malloc(sizeof(xStr));
    if(!sss) {
        printf("Cannot allocate struct for xStr\n");
        return NULL;
        }
    int capa = len + MEM_STEP;
    sss->str = malloc(capa);
    if(!sss) {
        printf("Cannot allocate str for xStr\n");
        free(sss);
        return NULL;
        }
    memset(sss->str, '\0', capa);
    sss->length = len;
    sss->capacity = capa;
    memcpy(sss->sentinel1, sent1, strlen(sent1));
    memcpy(sss->sentinel2, sent2, strlen(sent2));

    //printf("Created: %p\n", sss);

    // Add to xlist
    int filled = 0;
    for(int aa = 0; aa < sizeof(xlist) / sizeof(char*); aa++)
        {
        if(!xlist[aa])
            {
            xlist[aa] = (char*)sss; filled = 1;
            break;
            }
        }
    if(!filled)
        {
        printf("Warning: xlist overflow at %p\n", sss);
        }
    return sss;
}

int    xstr_cmp(xStr *sss, const xStr *str2)
{
    return memcmp(sss->str, str2->str, sss->length);
}

void    xstr_copy(xStr *sss, const char *str)
{
    int xlen = strlen(str);
    if (sss->capacity <= xlen)
        {
        free(sss->str);
        int capa = xlen + MEM_STEP;
        sss->str = malloc(capa);
        if(!sss->str)
            {
            printf("xstr_copy() alloc error\n");
            return;
            }
        sss->capacity = capa;
        }
    memcpy(sss->str, str, xlen);
    sss->str[xlen] = '\0';
    sss->length = xlen;
}

void    xstr_dup(xStr *sss, const xStr *str2)
{
    xstr_copy(sss, str2->str);
}

xStr    *xstr_fromstr(const char *str)

{
    xStr *sss = xstr_create(0);
    xstr_copy(sss, str);
    return sss;
}

void    xstr_cat(xStr *sss, const xStr *str2)
{
    if (sss->capacity <= sss->length + str2->length)
        {
        int capa = sss->length + str2->length + MEM_STEP;
        char *ppp = malloc(capa);
        if(!ppp)
            {
            printf("xstr_cat() alloc error\n");
            return;
            }
        memcpy(ppp, sss->str, sss->length);
        ppp[sss->length] = '\0';
        free(sss->str);
        sss->str = ppp;
        sss->capacity = capa;
        }
    memcpy(sss->str + sss->length, str2->str, str2->length);
    sss->str[sss->length + str2->length] = '\0';
    sss->length = sss->length + str2->length;
}

void    xstr_slice(xStr *sss, int beg, int end)

{
    if(beg >= sss->length)
        {
        printf("Invalid beg slice: %d\n", beg);
        return;
        }
    if(end >= sss->length)
        {
        printf("Invalid end slice: %d\n", end);
        return;
        }
    if(beg > end)
        {
        printf("Invalid slices: %d > %d\n", beg, end);
        return;
        }
    int size = end - beg;
    memcpy(sss->str, &sss->str[beg], size);
    sss->str[size] = '\0';
    sss->length = size;
}

void    xstr_catchar(xStr *sss, const char chh)
{
    if (sss->capacity <= sss->length + 1)
        {
        int capa = sss->length + 1 + MEM_STEP;
        char *ppp = malloc(capa);
        if(!ppp)
            {
            printf("xstr_cat() alloc error\n");
            return;
            }
        memcpy(ppp, sss->str, sss->length);
        ppp[sss->length] = '\0';
        free(sss->str);
        sss->str = ppp;
        sss->capacity = capa;
        }
    sss->str[sss->length] = chh;
    sss->str[sss->length + 1] = '\0';
    sss->length = sss->length + 1;
}

void    xstr_catstr(xStr *sss, const char *str)
{
    int xlen = strlen(str);
    if (sss->capacity <= sss->length + xlen)
        {
        int capa = sss->length + xlen + MEM_STEP;
        char *ppp = malloc(capa);
        if(!ppp)
            {
            printf("xstr_catstr() alloc error\n");
            return;
            }
        memcpy(ppp, sss->str, sss->length);
        ppp[sss->length] = '\0';
        free(sss->str);
        sss->str = ppp;
        sss->capacity = capa;
        }
    memcpy(&sss->str[sss->length], str, xlen);
    sss->str[sss->length + xlen] = '\0';
    sss->length = sss->length + xlen;
}

void    xstr_substr(xStr *sss, const xStr *str2, const xStr *str3, int offs)
{
    int prog = offs;  xStr *res = xstr_create(0);

    //printf("substr() '%s' -> '%s' with '%s' offs: %d\n",
    //                                        sss->str, str2->str, str3->str, offs);
    while(1)
        {
        if(prog >= sss->length)
            break;

        int found = 1;
        for(int aa = 0; aa < str2->length; aa++)
            {
            if(prog + aa >= sss->length) {
                //printf("beyond access prog %d aa %d\n", prog, aa);
                found = 0; break;
                }
            if(sss->str[prog + aa] != str2->str[aa]) {
                found = 0; break;
                }
            }
        if(found)
            {
            // subst
            xstr_cat(res, str3);
            prog += str2->length - 1;
            }
        else
            {
            xstr_catchar(res, sss->str[prog]);
            }
        prog++;
        }
    // reassign, remove old
    //printf("res: %s\n", res->str);
    free(sss->str);
    sss->str = res->str;
    sss->length = res->length;
    res->str = NULL;
    xstr_destroy(res);
}

xStr    *xstr_randstr(int len)
{
    xStr *res = xstr_create(0);

    for(int aa = 0; aa < len; aa++)
        {
        char chh = rand() & 0xff;
        xstr_catchar(res, chh);
        }
    return res;
}

xStr *xstr_hexdump(xStr *sss)

{
    int aa = 0;
    char dd[24];  dd[0] = '\0';
    char ss[24]; char cc[3];
    char *trailer =  "  | ";

    xStr *res = xstr_create(0);
    for(aa = 0; aa < sss->length; aa++)
        {
        if((aa % 16) == 0)
            {
            snprintf(ss, sizeof(ss), "0x%-3x ", aa);
            xstr_catstr(res, ss);
            }
        char chh = sss->str[aa];
        snprintf(ss, sizeof(ss), "%02x ", chh & 0xff);
        snprintf(cc, sizeof(cc), "%c", isprint(chh) ? chh : '.');
        strcat(dd, cc);
        if((aa % 16) == 15)
            {
            xstr_catstr(res, trailer);
            xstr_catstr(res, dd);
            xstr_catstr(res, trailer);
            dd[0] = '\0';
            snprintf(ss, sizeof(ss), "\n");
            }
        xstr_catstr(res, ss);
        }
    // Padd it at the end, if needed
    if(aa % 16)
        {
        for(int bb = 0; bb < 15 - (aa % 16); bb++)
            xstr_catstr(res, "   ");
        xstr_catstr(res, trailer);
        xstr_catstr(res, dd);
        for(int bb = 0; bb < 15 - (aa % 16); bb++)
            xstr_catstr(res, " ");
        xstr_catstr(res, " ");
        xstr_catstr(res, trailer);
        xstr_catstr(res, "\n");
        }
    return res;
}

// return -1 for no match, offset for match

int     xstr_strstr(xStr *sss, const xStr *str2, int offs)
{
    int ret = -1, prog = offs;

    //printf("strstr() '%s' -> '%s' offs: %d\n", sss->str, str2->str, offs);
    while(1)
        {
        if(prog >= sss->length)
            break;
        int found = 1;
        for(int aa = 0; aa < str2->length; aa++)
            {
            if(prog + aa >= sss->length)
                {
                //printf("beyond access prog %d aa %d\n", prog, aa);
                found = 0;
                break;
                }
            if(sss->str[prog + aa] != str2->str[aa])
                {
                found = 0;
                break;
                }
            }
        if(found)
            {
            ret = prog;
            break;
            }
        prog++;
        }
    return ret;
}

//int     xstr_strstr(xStr *sss, const xStr *str2, int offs)
//
//{
//    int ret = -1;
//    int prog = offs;
//
//    if (str2->str[0] == '\0')
//        return ret;
//
//    printf("strstr() '%s' -> '%s' offs: %d\n", sss->str, str2->str, offs);
//    while(1)
//        {
//        //printf("at: '%s'\n", sss->str + prog);
//        char *mmm = memchr(sss->str + prog, str2->str[0], sss->length - prog);
//        if(!mmm)
//            {
//            //printf("Not found\n");
//            break;
//            }
//        printf("mmm %p %ld len=%d prog=%d\n", mmm, mmm-sss->str, sss->length, prog);
//        if(mmm - (sss->str + prog + str2->length) >= sss->length)
//            {
//            printf("past end\n");
//            break;
//            }
//        //printf("cmp %p\n", mmm);
//        int xxx = strncmp(mmm, str2->str, str2->length);
//        //printf("xxx %d prog %d\n", xxx & 0xff, prog);
//        if(xxx == 0)
//            {
//            ret = mmm - sss->str + prog;
//            break;
//            }
//        else
//            {
//            prog = mmm - sss->str + 1;
//            }
//        }
//    return ret;
//}

xStr    *xstr_sprintf(char *format, ...)

{
    va_list ap;
    va_start(ap, format);
    int len = vsnprintf(NULL, 0, format, ap);
    va_end(ap);
    if(len < 0)
        return NULL;
    xStr *sss = xstr_create(len);
    if(sss == NULL)
        return NULL;
    va_start(ap, format);
    // Leave room for terminator
    len = vsnprintf(sss->str, len+1, format, ap);
    va_end(ap);
    if(len < 0)
        {
        xstr_destroy(sss);
        return NULL;
        }
    return sss;
}

int     xstr_len(xStr *sss)
{
    return(sss->length);
}

char    *xstr_ptr(xStr *sss)
{
    return(sss->str);
}

void    xstr_destroy(xStr *sss)

{
    int ret  = memcmp(sss->sentinel1, sent1, strlen(sent1));
    if(ret) {
        printf("Bad sentinel1\n");
        }
    int ret2 = memcmp(sss->sentinel2, sent2, strlen(sent2));
    if(ret2) {
        printf("Bad sentinel2\n");
        }
    if(sss->str)
        free(sss->str);
    // Remove from to xlist
    for(int aa = 0; aa < sizeof(xlist) / sizeof(char*); aa++)
        {
        if(xlist[aa] == (char*)sss)
            {
            xlist[aa] = NULL;
            break;
            }
        }
    free(sss);
}

void    xstr_dumplist()

{
    for(int aa = 0; aa < sizeof(xlist) / sizeof(char*); aa++)
        {
        if(xlist[aa])
            printf("%d %p\n", aa, xlist[aa]);
            //printf("%p %s\n", xlist[aa], ((xStr*)xlist[aa])->str);
        }
}

// pad buffx at regular freq.

void    xstr_padbr(xStr *sss, char *buffx, char *pad, int freq)

{
    //printf("xstr_padbr() [%s] '%s' '%s' %d\n", sss->str, buffx, pad, freq);
    int dd = 0, xlen = strlen(buffx), padlen = strlen(pad);
    for (int bb = 0; bb < xlen; bb++)
        {
        char chh = buffx[bb];
        xstr_catchar(sss, chh);
        // Check for pad
        if(bb + padlen < xlen && strncmp(&buffx[bb], pad, padlen) == 0)
            {
            //printf("match: '%4s' org: '%s'\n", &buffx[bb], pad);
            dd = -padlen + 1;
            }
        else
            {
            dd++;
            }
        // Inject <br>
        if(dd >= freq)
            {
            //printf("Add at %d\n", bb);
            dd = 0;
            xstr_catstr(sss, pad);
            }
        }
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

// EOF
