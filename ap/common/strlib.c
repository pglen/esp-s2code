
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

static char *TAG= "strlib";


typedef struct _xStr

{
    char sentinel1[4];
    int   length;
    int   capacity;
    char *str;
    char sentinel2[4];

} xStr;

static char *sent1 = "1234";
static char *sent2 = "5678";

xStr *xstr_create(int len)

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
    return sss;
}

xStr *xstr_fromstr(const char *str)

{
    xStr *sss = xstr_create(0);
    xstr_copy(sss, str);
    return sss;
}

void    xstr_dup(xStr *sss, const xStr *str2)
{
    xstr_copy(sss, str2->str);
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

void    xstr_add(xStr *sss, const xStr *str2)
{
    int xlen = strlen(str2->str);
    if (sss->capacity <= sss->length + xlen)
        {
        char *ppp = malloc(sss->length + xlen + 1);
        if(!ppp)
            {
            printf("xstr_add() alloc error\n");
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

void xstr_destroy(xStr *sss)

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
    free(sss);
}

#ifdef LINUX_TEST

void  main()
{
    printf("Strlib test\n");
    xStr *str1 = xstr_create(0);
    xStr *str2 = xstr_create(0);
    xStr *str3 = xstr_fromstr("1234");

    printf("Strlib %p\n", str1);
    xstr_copy(str1, "Hello Hello Hello Hello Hello ");
    printf("str1 '%s'\n", str1->str);
    xstr_copy(str2, "World World World World ");
    printf("str2 '%s'\n", str2->str);
    xstr_add(str1, str2);
    printf("add '%s'\n", str1->str);

    xstr_dup(str1, str2);
    printf("dup '%s'\n", str1->str);
    printf("cmp '%d'\n", xstr_cmp(str1, str2));

    xstr_add(str1, str3);
    printf("add3 '%s'\n", str1->str);
    printf("cmp '%d'\n", xstr_cmp(str1, str2));

    xstr_destroy(str1);
    xstr_destroy(str2);
}

#endif

