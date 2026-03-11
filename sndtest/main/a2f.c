
/* =====[ esp_loratest.sess ]========================================================

   File Name:       a2f.c

   Description:     Functions for a2f.c

   Revisions:

      REV   DATE                BY              DESCRIPTION
      ----  -----------         ----------      --------------------------
      0.00  Sat 17.Jan.2026     Peter Glen      Initial version.
      0.00  Wed 28.Jan.2026     Peter Glen

   ======================================================================= */

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

int gl_cnt = 0;

// Get coefficients from python program
// use: calc_power.py

#define OUTP_SIZE 96
int res[OUTP_SIZE]  = {0, };
int res2[OUTP_SIZE]  = {0, };
int valarr[OUTP_SIZE] = { 98,  97,  96,  95,  94,  93,  92,  91,  90,
                  89,  88,  87,  86,  85,  84,  83,  82,  81,
                  80,  79,  78,  77,  76,  75,  74,  73,  72,
                  71,  70,  69,  68,  67,  66,  65,  64,  63,
                  62,  61,  60,  59,  58,  57,  56,  55,  54,
                  53,  52,  51,  50,  49,  48,  47,  46,  45,
                  44,  43,  42,  41,  40,  39,  38,  37,  36,
                  35,  34,  33,  32,  31,  30,  29,  28,  27,
                  26,  25,  24,  23,  22,  21,  20,  19,  18,
                  17,  16,  15,  14,  13,  12,  11,  10,  9,
                  8,  7,  6,  5,  4,  3,  };

#define RES_LEN (sizeof(res)/sizeof(int))

static void    framezero(int16_t  *buffer, int len)
{
    // Find zero crossing from begin and from end
    int16_t prev = buffer[0];
    int aa, ccc, ddd;
    for (aa = 1; aa < len / 8; aa++) {
        if (buffer[aa] > 0 && prev <= 0)
            {
            //printf("pos %d %d %d ", aa, prev, buffer[aa]);
            break;
            }
      if (buffer[aa] <= 0 && prev > 0)
            {
            //printf("neg %d %d %d ", aa, prev, buffer[aa]);
            break;
            }
    prev = buffer[aa];
    }
    for (ccc = 0; ccc < aa; ccc++)
        buffer[ccc] = 0;

    // Find zero crossing from end
    int16_t xprev = buffer[len-1];
    for (aa = len-2; aa >= 7 * len / 8; aa--) {
        if (buffer[aa] > 0 && xprev <= 0)
            {
            //printf("xpos %d %d %d ", aa, prev, buffer[aa]);
            break;
            }
      if (buffer[aa] <= 0 && xprev > 0)
            {
            //printf("xneg %d %d %d ", aa, prev, buffer[aa]);
            break;
            }
    xprev = buffer[aa];
    }
    for (ddd = len-1; ddd >= aa; ddd--)
        buffer[ddd] = 0;

    //printf("ccc=%d ddd=%d\n", ccc, ddd);
}

void a2f(int16_t *buffer, int len, int *outp, int olen)
{
    //printf("buff %p len=%d\n", buffer, len);
    //uint64_t start = esp_timer_get_time();

    framezero(buffer, len);

    #if 0      // Dump
        int bbbb = 0;
        for (int aa = 0; aa < len; aa++) {
            if (1) { //(aa % 4 == 0) {
                printf("%04d ", buffer[aa]);
                if (bbbb % 12 == 11) printf("\n");
                bbbb++;
            }
        }
        printf("\n\n");
    #endif

    #if 0
    // Show values as horizontal bars (to confirm A2D op)
    char dashx[] =
    "-------------------------------------------------------------";
    for (int aa = 0; aa < len; aa++) {
            int idxx = (buffer[aa] + 2000) / 200;   // Scale
            idxx = sizeof(dashx) - idxx;            // Invert
            idxx = idxx < 0 ? 0 : idxx;             // Limit
            //printf("idxx3 %d ", idxx);
            printf("%s %d %x\n", &(dashx[idxx]), buff[aa], buff[aa]);
     }
    #endif
    // Square wave FFT
    int tmp = 0;
    int16_t sigx = 0;

    for (int aa = 0; aa < len; aa++)
        {
        sigx = buffer[aa];
        //printf("sigx %d\n", sigx);
        for (int bb = 0; bb < sizeof(res)/sizeof(int); bb++)
            {
            //printf("bb %d\n", bb);
            tmp = aa % valarr[bb];
            if(tmp <= valarr[bb]/2)
                res[bb] += sigx;
            else
                res[bb] -= sigx;

            // Also calculate 90 degrees off -- NO USE
            //if(tmp >= valarr[bb]/4 && tmp <= 3*valarr[bb]/4)
            //    res2[bb] += sigx;
            //else
            //    res2[bb] -= sigx;
            }
        }
    // Normalize, abs
    for (int bb = 0; bb < sizeof(res)/sizeof(int); bb++)
        {
        res[bb] /= valarr[0];
        res[bb] = llabs(res[bb]);
        }
    // Copy OUT
    for (int bb = 0; bb < sizeof(res)/sizeof(int); bb++)
        {
        outp[bb] = res[bb];
        }
    gl_cnt++;

    //uint64_t end = esp_timer_get_time();
    //printf("analize timing: %lld us ", end - start);
}

// EOF
