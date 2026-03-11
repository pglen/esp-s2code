
/* =====[ access point template project ]=================================

   File Name:       common.c

   Description:

   Revisions:

      REV       DATE               BY          DESCRIPTION
      ----  -----------         ----------      -------------------------
      0.00  Tue 10.Mar.2026     Peter Glen      Initial version.

   ======================================================================= */

#define     MAX_PAIRED_DEVICES  4

#define     MAX_PAIR_TIME       15      // Seconds to talk / listen
#define     NUM_RETRIES         2       // ESPNOW retries (minimum ONE)
#define     NUM_BURSTS          1       // Micro bursts (minimum ONE)

// Repeat switch break (relay off) transactions this many times
#define MAX_NUM_BURSTS      3

extern  int         gl_bursts;
extern  char        *gl_pass;
extern  int         gl_battery;
extern  int         gl_alive;
extern  int         gl_packon;

//extern  int         gl_broad;
extern  int         gl_listen;

extern  uint32_t        gl_paired[MAX_PAIRED_DEVICES];
extern  int             gl_fresh_pair;

void    init_trans();
void    start_listening();
void    start_pairing();
void    trans_one_cycle();
void    send_trans(int payload, int payload2, int batt);
int     is_rf_busy();

// EOF