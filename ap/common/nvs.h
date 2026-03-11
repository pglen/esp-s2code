
/* =====[ access point template project ]=================================

   File Name:       common.c

   Description:

   Revisions:

      REV       DATE               BY          DESCRIPTION
      ----  -----------         ----------      -------------------------
      0.00  Tue 10.Mar.2026     Peter Glen      Initial version.

   ======================================================================= */

int     submit_nvs_int(const char *name, int valx);
int     submit_nvs_int64(const char *name, int64_t valx);
int     submit_nvs_str(const char *name, const char *strx);
int     submit_nvs_float(const char *name, float valx);

void    init_nvs_writer();

// EOF



