#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/param.h>
#include <time.h>
#include <stdint.h>
#include <errno.h>

#include "strlib.h"

// Get nano seconds

int64_t nanos()

{
    int64_t nanoseconds = -1; struct timespec ts;
    int ret = clock_gettime(CLOCK_REALTIME, &ts);
    if (ret >= 0) {
        // Calculate total nanoseconds
        nanoseconds = (int64_t)ts.tv_sec * 1000000000L + (int64_t)ts.tv_nsec;
    }
    //printf("Nanoseconds since epoch: %ld %d ret=%d\n", nanoseconds, errno, ret);
    return  nanoseconds;
}

int64_t docalib()

{
    int64_t nanos1 = nanos();
    int64_t nanos2 = nanos();
    int64_t calibx = (nanos2 - nanos1) / 1000;
    return calibx;
}

void  main()
{
    int64_t calib = docalib();
    //printf("Time delta calib: %ld us\n", calib);

    //printf("Strlib test\n");
    xStr *str1 = xstr_create(0);

    //xstr_catchar(str1, 'x');
    //printf("%d '%s'\n", str1->length, str1->str);
    //printf("bad access: '%s'\n", &str1->str[str1->length+32]);
    //xstr_destroy(str1);
    //exit(0);

    xStr *str2 = xstr_create(0);
    xStr *str3 = xstr_fromstr("1234");

    //printf("Strlib %p\n", str1);
    xstr_copy(str1, "Hello Hello ");
    printf("str1: '%s' len: %d ", str1->str, str1->length);
    //printf("str1 len: %d %ld\n", str1->length, strlen(str1->str));
    xstr_copy(str2, "World World ");
    printf("str2: '%s' len: %d ", str2->str, str2->length);
    printf("str3: '%s'\n", str3->str);
    xstr_cat(str1, str2);
    printf("cat '%s' len: %d\n", str1->str, str1->length);
    xstr_catstr(str1, "again");
    printf("catstr '%s' len: %d\n", str1->str, str1->length);
    xstr_dup(str1, str2);
    printf("dup '%s'  len: %d\n", str1->str, str1->length);
    printf("cmp res=%d\n", xstr_cmp(str1, str2));

    xstr_cat(str1, str3);
    printf("add1+3: '%s'\n", str1->str);
    printf("cmp res=%d\n", xstr_cmp(str1, str2));

    // test efence
    //str1->str[str1->length + 11] = 'a';

    int cnt = 0;
    for(int aa = 0; aa < 3; aa++)
        {
        xStr *strx =  xstr_sprintf("Hello: %d", cnt++);
        printf("%s ", strx->str);
        xstr_destroy(strx);
        }
    xStr *str4 =  xstr_sprintf("Hello: %s 567 %s", "pur 56se", "test 567 end");
    printf("\nlen: %d %ld '%s'\n", xstr_len(str4),
                        strlen(xstr_ptr(str4)), xstr_ptr(str4));
    xStr *str5 = xstr_fromstr("567");

    int offs = 0;
    while(1)
        {
        offs = xstr_strstr(str4, str5, offs);
        if(offs < 0) break;
        printf("strstr %d '%s'\n", offs, &str4->str[offs]);
        offs++;
        }
    xStr *str6 = xstr_fromstr("wqqw");
    xstr_substr(str4, str5, str6, 0);
    printf("substr len=%d '%s'\n", str4->length, str4->str);

    xstr_subststr(str4, " ", " X ", 0);
    printf("subststr len=%d '%s'\n", str4->length, str4->str);

    int64_t nanos3 = nanos();
    srand(time(NULL));
    xStr *str8 = xstr_randstr(32);
    int64_t nanos4 = nanos();
    //printf("Time delta rand %ld us\n", (nanos4 - nanos3)/1000);
    xStr *str9 = xstr_hexdump(str8);
    int64_t nanos5 = nanos();
    //printf("Time delta %ld us\n", (nanos5 - nanos4)/1000);

    printf("%s", str9->str);
    xstr_copy(str1, "");

    printf("pad org: [%s]\n", str4->str);
    xstr_padbr(str1, str4->str, "\n", 12);
    printf("pad:\n%s\n", str1->str);

    xstr_slice(str4, 4, 8);
    printf("slice  len=%d '%s'\n", str4->length, str4->str);

    xstr_destroy(str1);
    xstr_destroy(str2);
    xstr_destroy(str3);
    xstr_destroy(str4);
    xstr_destroy(str5);
    xstr_destroy(str6);
    xstr_destroy(str8);
    xstr_destroy(str9);

    //printf("Post dump:\n");
    xstr_dumplist();
}

// EOF
