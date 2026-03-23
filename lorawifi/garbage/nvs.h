
/* =====[ Preventive Maintainance counter ]===============================

   File Name:       nvs.h

   Description:     NVS utility functions for PMC

   Revisions:

      REV   DATE                BY              DESCRIPTION
      ----  -----------         ----------      --------------------------
      0.00  Tue 09.Feb.2021     Peter Glen      Initial
      0.00  Wed 31.Mar.2021     Peter Glen      Copied from pmc

   ======================================================================= */

int     submit_nvs_int(const char *name, int valx);
int     submit_nvs_int64(const char *name, int64_t valx);
int     submit_nvs_str(const char *name, const char *strx);
int     submit_nvs_float(const char *name, float valx);

void    init_nvs_writer();

// EOF



