/* =====[ access point template project ]=================================

   File Name:       httpd.h

   Description:

   Revisions:

      REV       DATE               BY          DESCRIPTION
      ----  -----------         ----------      -------------------------
      0.00  Wed 25.Mar.2026     Peter Glen      Initial version.

   ======================================================================= */

#define DEF_SPREAD    "10"
#define DEF_BWIDTH    "50"
#define DEF_POWER     "12"
#define DEF_FREQ      "433.375"
#define DEF_TRENCH    "0"

#define SENTMAX 24        // Maximum history items

extern  int     gl_recala      ;
extern  int     gl_sentprog    ;

extern  char    gl_spread[32]   ;
extern  char    gl_bwidth[32]   ;
extern  char    gl_txpower[32]  ;
extern  char    gl_txfreq[32]   ;
extern  char    gl_deftren[32]  ;
extern  char    gl_curr_tr[32]  ;
extern  char    gl_statx[32] ;
extern  int     gl_rssi ;
extern  int     gl_freq_err ;
extern  int     gl_update ;
extern  int     gl_update2 ;
extern  double  gl_ppm ;
extern  double  gl_devi ;
extern  char    gl_ack[16] ;
extern  char    *gl_sentbuff;
extern  int     gl_sendtrench ;
extern  char    *histx ;
extern  const   char *gl_sentarr[SENTMAX] ;
extern  int     gl_prom ;

extern  char    gl_netname[32] ;
extern  char    gl_netpass[32] ;

// Make it permanent
#define NVS_WRAP 24

extern  void    stop_webservers(void);
extern  void    start_webservers(void);
extern  void    add_hist(const char *item, int noperm);

// EOF
