
/* =====[ access point template project ]=================================

   File Name:       comline.h

   Description:

   Revisions:

      REV       DATE               BY          DESCRIPTION
      ----  -----------         ----------      -------------------------
      0.00  Wed 25.Mar.2026     Peter Glen      Initial version.

   ======================================================================= */

#define DEF_SPREAD    "10"
#define DEF_BWIDTH    "50E3"
#define DEF_POWER     "12"
#define DEF_FREQ      "433.375E6"
#define DEF_TRENCH    "0"

extern char    gl_spread[32]   ;
extern char    gl_bwidth[32]   ;
extern char    gl_txpower[32]  ;
extern char    gl_txfreq[32]   ;
extern char    gl_deftren[32]  ;
extern char    gl_curr_tr[32]  ;

void    out_str(const char *strx, const char *str2);
void    set_split();
void    start_console(char *prompt);


// EOF
