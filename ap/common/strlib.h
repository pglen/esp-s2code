
/* =====[ ap.sess ]========================================================

   File Name:       untitled_1.txt

   Description:     Functions for untitled_1.txt

   Revisions:

      REV   DATE                BY              DESCRIPTION
      ----  -----------         ----------      --------------------------
      0.00  Tue 17.Mar.2026     Peter Glen      Initial version.

   ======================================================================= */

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
void    xstr_destroy(xStr *sss);
xStr    *xstr_sprintf(char *format, ...);
int     xstr_len(xStr *sss);
char    *xstr_ptr(xStr *sss);
int     xstr_strstr(xStr *sss, const xStr *str2, int offs);
void    xstr_dumplist();


// EOF
