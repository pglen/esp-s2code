// header for led blinkers

//extern  int     blink_cnt;
//extern  int     rr, gg, bb;
extern  int     delay, delay2;

void    blink_led(int cnt, int  rrr, int ggg, int  bbb);
void    pulse_led(int ms, int rr, int gg, int bb);
void    toggle_led(int okcol);
void    configure_led(void);

// EOF
