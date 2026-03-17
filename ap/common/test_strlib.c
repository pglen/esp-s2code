#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/param.h>

#include "strlib.h"

void  main()
{
    //printf("Strlib test\n");
    xStr *str1 = xstr_create(0),*str2 = xstr_create(0);
    xStr *str3 = xstr_fromstr("1234");

    //printf("Strlib %p\n", str1);
    xstr_copy(str1, "Hello Hello ");
    printf("str1: '%s' len: %d %ld ", str1->str, str1->length, strlen(str1->str));
    xstr_copy(str2, "World World ");
    printf("str2: '%s'\n", str2->str);
    xstr_cat(str1, str2);
    printf("add '%s' len: %d %ld\n", str1->str, str2->length, strlen(str2->str));

    xstr_dup(str1, str2);
    printf("dup '%s'  len: %d %ld \n", str1->str, str1->length, strlen(str1->str));
    printf("cmp res=%d\n", xstr_cmp(str1, str2));

    xstr_cat(str1, str3);
    printf("add3 '%s'\n", str1->str);
    printf("cmp res=%d\n", xstr_cmp(str1, str2));

    // test efence
    //str1->str[str1->length + 11] = 'a';

    xStr *str4 =  xstr_sprintf("Hello: %s 56 '%s'", "pur 56 se", "test56");
    printf("len: %d %ld '%s'\n", xstr_len(str4),
                        strlen(xstr_ptr(str4)), xstr_ptr(str4));
    xStr *str5 = xstr_fromstr("56");

    int offs = 0;
    while(1)
        {
        offs = xstr_strstr(str4, str5, offs);
        if(offs < 0) break;
        printf("strstr %d '%s'\n", offs, &str4->str[offs]);
        offs++;
        }
    //printf("Pre:\n");
    //xstr_dumplist();

    xstr_destroy(str1);
    xstr_destroy(str2);
    xstr_destroy(str3);
    xstr_destroy(str4);
    xstr_destroy(str5);

    //printf("Post:\n");
    xstr_dumplist();
}

