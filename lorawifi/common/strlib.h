
/* =====[ ap.sess ]========================================================

   File Name:       untitled_1.txt

   Description:     Functions for untitled_1.txt

   Revisions:

      REV   DATE                BY              DESCRIPTION
      ----  -----------         ----------      --------------------------
      0.00  Tue 17.Mar.2026     Peter Glen      Initial version.

   ======================================================================= */

#define    MEM_STEP     48      // Alloc this much more

typedef struct _xStr
{
    char sentinel1[4];
    int   length;
    int   capacity;
    char *str;
    char sentinel2[4];

} xStr;

xStr    *xstr_create(int len);
int     xstr_cmp(xStr *sss, const xStr *str2);
void    xstr_copy(xStr *sss, const char *str);
void    xstr_dup(xStr *sss, const xStr *str2);
xStr    *xstr_fromstr(const char *str);
void    xstr_cat(xStr *sss, const xStr *str2);
void    xstr_catchar(xStr *sss, const char chh);
void    xstr_destroy(xStr *sss);
xStr    *xstr_sprintf(char *format, ...);
int     xstr_len(xStr *sss);
char    *xstr_ptr(xStr *sss);
xStr    *xstr_hexdump(xStr *sss);
xStr    *xstr_randstr(int len);
int     xstr_strstr(xStr *sss, const xStr *str2, int offs);
void    xstr_substr(xStr *sss, const xStr *str2, const xStr *str3, int offs);
void    xstr_subststr(xStr *sss, const char *str2, const char *str3, int offs);
void    xstr_slice(xStr *sss, int beg, int end);
void    xstr_catstr(xStr *sss, const char *str);
void    xstr_dumplist();
void    xstr_padbr(xStr *sss, char *buffx, char *pad, int freq);
xStr    *xstr_dumpbuff(const char *mem, int xlen);
xStr    *xstr_frombuff(const char *buff, int xlen);

// EOF
