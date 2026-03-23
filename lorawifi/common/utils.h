
/* =====[ access point template project ]=================================

   File Name:       common.c

   Description:

   Revisions:

      REV       DATE               BY          DESCRIPTION
      ----  -----------         ----------      -------------------------
      0.00  Tue 10.Mar.2026     Peter Glen      Initial version.

   ======================================================================= */

#define FALSE 0
#define TRUE 1

#define CURR_TIME   time2str(esp_timer_get_time())

typedef  const char cchar;
typedef  const unsigned char cuchar;

// /#define MIN(aa, bb)  ((aa) < (bb) ? (aa) : (bb))
// /#define MAX(aa, bb)  ((aa) > (bb) ? (aa) : (bb))

char    *xstrdup(const char *str);
char    *subst_str(char *orgx, const char *substx, const char *restr);
char    *subst_str_at(char *orgx, cchar *atstr, cchar *substx, cchar *restr);
void    preprocess_string(char* str);
int     print_posts(const char *names[], char *mems[]);
int     parse_post(const char *buf, const char *names[], char *mems[], int xlen);
int     unescape_url(char *str, char *strout, int lims);
void    cut_tailspace(char* str);
double  atofx(const char *s);
char    *get_arg_ptr(const char *keys);
int     dec_str(char *str, int len, char *key, int klen, char *out, int olen);
int     enc_str(char *str, int len, char *key, int klen, char *out, int olen);
char    *time2str(int64_t ttt);
char    *xsnprintf(char *format, ...);
int     inc_bootcount();

// EOF
