
/* =====[ AKOSTAR IOCOM project ]====================================

   File Name:       wifi.c

   Description:

   Revisions:

      REV       DATE                BY          DESCRIPTION
      ----  -----------         ----------      -------------------------
      0.00  Mon 29.Mar.2021     Peter Glen      Initial version.

   ======================================================================= */

extern  char    gl_netname[32];
extern  char    gl_netpass[32];

extern  int     gl_wifi_on;
extern int      gl_wifi_trans;

void    start_webpage();
void    wifi_initial(int startap);
int     wifi_sleep_wake(int wakeflag);
void    wifi_wait_ready();

// EOF
