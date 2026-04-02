
/* =====[ access point template project ]=================================

   File Name:       packets.h

   Description:

   Revisions:

      REV       DATE               BY          DESCRIPTION
      ----  -----------         ----------      -------------------------
      0.00  Wed 25.Mar.2026     Peter Glen      Initial version.

   ======================================================================= */

typedef  unsigned char uchar;

extern  SemaphoreHandle_t sSemaphore;
extern  SemaphoreHandle_t hSemaphore;
extern  SemaphoreHandle_t iSemaphore;

int16_t lora_chksum(const char *str, int len);
int     assemble_packet(const char *pay, uint16_t trench, char *outstr, int maxlen);
int     disass_packet(const char *instr, uint16_t *hash, uint16_t *trench, const char **out);
int     check_packet(const char *str, int len);
void    send_payload(const char * buff, uint16_t trench);
int     isalnum2(char chh);
void    sdump(const char *str, int len, char *out, int maxlen);
void    hexdump(const char *str, int len, char *out, int maxlen);
void    init_lora_common();

// EOF
