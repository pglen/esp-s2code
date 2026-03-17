
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

static char *TAG= "strlib";

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
    sss->str = malloc(len + 1);
    if(!sss) {
        printf("Cannot allocate str for xStr\n");
        free(sss);
        return NULL;
        }
    memset(sss->str, '\0', len + 1);
    sss->length = len;
    sss->capacity = len;
    memcpy(sss->sentinel1, sent1, strlen(sent1));
    memcpy(sss->sentinel2, sent2, strlen(sent2));

    //printf("Created: %p\n", sss);

    // Add to xlist
    int filled = 0;
    for(int aa = 0; aa < sizeof(xlist) / sizeof(char*); aa++)
        {
        if(!xlist[aa])
            {
            //printf("Added: %p\n", sss);
            xlist[aa] = (char*)sss;
            filled = 1;
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
        sss->str = malloc(xlen + 1);
        if(!sss->str)
            {
            printf("xstr_copy() alloc error\n");
            return;
            }
        sss->capacity = xlen;
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
    int xlen = strlen(str2->str);
    if (sss->capacity <= sss->length + xlen)
        {
        char *ppp = malloc(sss->length + xlen + 1);
        if(!ppp)
            {
            printf("xstr_cat() alloc error\n");
            return;
            }
        memcpy(ppp, sss->str, sss->length);
        ppp[sss->length] = '\0';
        free(sss->str);
        sss->str = ppp;
        sss->capacity = sss->length + xlen;
        }
    memcpy(sss->str + sss->length, str2->str, xlen);
    sss->str[sss->length + xlen] = '\0';
    sss->length = sss->length + xlen;
}

// return -1 for no match, offset for match

int     xstr_strstr(xStr *sss, const xStr *str2, int offs)
{
    int ret = -1;
    int prog = offs;

    while(1)
        {
        if(prog >= sss->length)
            break;
        int found = 1;
        for(int aa = 0; aa < str2->length; aa++)
            {
            if(prog + aa >= sss->length)
                {
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

// EOF
