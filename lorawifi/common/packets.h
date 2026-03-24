// packets

extern SemaphoreHandle_t sSemaphore;

void  set_split();
int16_t lora_chksum(const char *str, int len);
int     assemble_packet(const char *pay, uint16_t trench, char *outstr, int maxlen);
int     disass_packet(const char *instr, int inlen, uint16_t *hash, uint16_t *trench);
int     check_packet(const char *str, int len);
void    send_payload(const char * buff);
int     isalnum2(char chh);
void    sdump(const char *str, int len, char *out, int maxlen);
void    hexdump(const char *str, int len, char *out, int maxlen);
void    init_lora_common();

// EOF
