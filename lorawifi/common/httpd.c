
/* =====[ ap.sess ]========================================================

   File Name:       main.c

   Description:     Functions for main.c

   Revisions:

      REV   DATE                BY              DESCRIPTION
      ----  -----------         ----------      --------------------------
      0.00  Tue 03.Mar.2026     Peter Glen      Initial version.

   ======================================================================= */

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <linenoise/linenoise.h>
#include <argtable3/argtable3.h>
#include <esp_console.h>
#include <esp_timer.h>
#include <esp_https_server.h>
#include <esp_http_server.h>
#include <esp_tls.h>

#include <lwip/err.h>
#include <lwip/sys.h>
#include <esp_heap_caps.h>
#include <driver/gpio.h>

#include "cJSON.h"

#include "protocol.h"
#include "utils.h"
#include "common.h"
#include "wifi.h"
#include "nvs.h"
#include "strlib.h"
#include "packets.h"
#include "comline.h"
#include "httpd.h"

#include "lora.h"
#include "leds.h"

char    gl_netname[32] = {0, };
char    gl_netpass[32] = {0, };

static const char *TAG = "lorawifi";

static httpd_handle_t gl_serverx = NULL;
static httpd_handle_t gl_server = NULL;
static int timeout_cnt = 0;

char *statstr   = "StatusStatusStatusStatusStatusStatusStatusStatus";
char *rebostr   = "RebootRebootRebootRebootRebootRebootRebootRebootReboot";
char *substrx   = "sssssssssssssssssssssssssssssss";
char *substrxx  = "xxxxxx";
char *checkstr  = "checked";
char *softstr   = "softversoftversoftversoftversoftversoftver";
char *macstr    = "macmacmacmacmacmacmac";
char *comstr    = "comstatcomstatcomstatcomstatcomstat";
char *bcstr     = "bcountbcount";
char *netstr    = "hnamehnamehnamehnamehnamehnamehname";
char *passstr   = "pass1pass1pass1pass1pass1pass1";
char *pass2str  = "pass2pass2pass2pass2pass2pass2";
char *spreadstr = "spreadspread";
char *bandstr   = "bandbandbandband";
char *txpowstr  = "txpowtxpowtxpow";
char *freqstr   = "freqfreqfreqfreqfreqfreq";
char *chanstr   = "deftrenchdeftrench";
char *trenchstr = "trenchtrenchtrench";
char *tunestr   = "rxtunerxtunerxtune";

const   char *gl_sentarr[SENTMAX] = {0, };
char    *histx = "curr_hist";
char    *gl_sentptr = NULL;
uint64_t    gl_last_http = 0;

void    add_hist(const char *item, int noperm)

{
    if(!item)
        return;

    //printf("History: '%s'\n", item);
    char *tmp = xstrdup(item);
    if(!tmp)
        {
        printf("Cannot alloc for hist\n");
        return;
        }
    //printf("tmp: %s\n", tmp);
    TAKE_SEMA(hSemaphore, TAG, portMAX_DELAY);
    // Cut to size
    if(gl_sentprog >= SENTMAX)
        {
        free((void*)gl_sentarr[0]);
        // Shift down
        for(int aa = 1; aa < gl_sentprog; aa++)
            {
            gl_sentarr[aa-1] = gl_sentarr[aa];
            }
        gl_sentprog--;
        }
    gl_sentarr[gl_sentprog] = tmp;
    gl_sentprog++;
    //int     freex = get_nvs_free();
    //printf("free: %d \n", freex);
    if (!noperm)
        {
        nvs_handle my_handle;
        esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error (%d) opening NVS handle!", err);
            goto err3;
            }
        int16_t curr_hist = 0;
        err = nvs_get_i16(my_handle, histx, &curr_hist);
        //printf("curr_hist %d\n", freex, curr_hist);
        nvs_close(my_handle);
        if(curr_hist >= NVS_WRAP)
            {
            curr_hist = 0;
            }
        char head[24];
        snprintf(head, sizeof(head), "head_%d", curr_hist);
        //printf("Submit: %s -- %s\n", head, tmp);
        submit_nvs_str(head, tmp);

        curr_hist++;
        submit_nvs_short(histx, curr_hist);

        // Sleep for the values to commit
        //vTaskDelay(300 / portTICK_PERIOD_MS);

      err3:
        nvs_close(my_handle);
        }
    GIVE_SEMA(hSemaphore);
}

static  const char *tmperr = "Error reg %s URI handler! %x";

void    httpd_register_uri(const httpd_uri_t *urivar)

{
    esp_err_t esp_ret = 0;

    esp_ret = httpd_register_uri_handler(gl_server, urivar);
    if (esp_ret != ESP_OK) {
        printf(tmperr, urivar->uri, esp_ret);
        }
    esp_ret = httpd_register_uri_handler(gl_serverx, urivar);
    if (esp_ret != ESP_OK) {
        printf(tmperr, urivar->uri, esp_ret);
        }
}

#include "h/page_x.h"

static  void    fill_version(char *mem)
{
    subst_str(mem, softstr, gl_version);
}

static  void    fill_comstr(char *mem)
{
    //char vtmp[48];
    //snprintf(vtmp, sizeof(vtmp), "%s %s", PROG_VER, PROG_DATE);
    subst_str(mem, comstr, "");
}

static  void    fill_bcnt(char *mem)
{
    char vtmp[48];
    int bcnt = inc_bootcount();
    snprintf(vtmp, sizeof(vtmp), "%d", bcnt);
    subst_str(mem, bcstr, vtmp);
}

static  void    fill_mac(char *mem)
{
    char vtmp[48];
    GET_MAC(self_mac)
    snprintf(vtmp, sizeof(vtmp), "%02X:%02X:%02X:%02X:%02X:%02X",
         self_mac[0], self_mac[1], self_mac[2], self_mac[3],
            self_mac[4], self_mac[5]);
    subst_str(mem, macstr, vtmp);
}

static  void    fill_status(char *mem)
{
    char vtmp[48];
    snprintf(vtmp, sizeof(vtmp), "%s", "Connected with board.");
    subst_str(mem, statstr, vtmp);
}

void    subst_footer(char *mem)

{
    fill_status(mem);
    fill_mac(mem);
    fill_version(mem);
    fill_bcnt(mem);
    fill_comstr(mem);
}

#include "h/page_8.h"

/* An HTTP GET handler */
static esp_err_t manual_get_handler(httpd_req_t *req)

{
    gl_last_http = esp_timer_get_time();

    char* resp_str = xstrdup(manual_html);
    if(!resp_str)
        return ESP_FAIL;

    subst_footer(resp_str);

    //printf("manual.html incoming query %s\n", req->uri);
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    free(resp_str);
    return ESP_OK;
}

static httpd_uri_t manpage = {
    .uri       = "/page_8.html",
    .method    = HTTP_GET,
    .handler   = manual_get_handler,
};

#include "h/page_3.h"

static esp_err_t lora_get_handler(httpd_req_t *req)

{
    gl_last_http = esp_timer_get_time();
    char *resp_str = xstrdup(lora_html);
    if(!resp_str)
        return ESP_FAIL;

    // Send current parameters
    char tmp[24];
    subst_str(resp_str, statstr,    "");
    subst_str(resp_str, spreadstr,  gl_spread    );
    subst_str(resp_str, txpowstr,   gl_txpower   );

    double bbb = atofx(gl_bwidth); if(bbb > 1000) bbb /= 1000;
    snprintf(tmp, sizeof(tmp), "%d", (int)bbb);
    subst_str(resp_str, bandstr,    tmp         );

    double ddd = atofx(gl_txfreq); if(ddd > 1000000) ddd /= 1000000;
    snprintf(tmp, sizeof(tmp), "%d", (int)ddd);
    subst_str(resp_str, freqstr,    tmp    );

    subst_str(resp_str, tunestr,    gl_corrfreq );

    subst_footer(resp_str);
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    free(resp_str);
    return ESP_OK;
}

static httpd_uri_t lorapage = {
    .uri       = "/page_3.html",
    .method    = HTTP_GET,
    .handler   = lora_get_handler,
};

#include "h/page_4.h"

static esp_err_t conf_get_handler(httpd_req_t *req)

{
    gl_last_http = esp_timer_get_time();
    char *resp_str = xstrdup(config_html);
    if(!resp_str)
        return ESP_FAIL;

    // Send current parameters
    subst_str(resp_str, statstr,    "");
    subst_str(resp_str, chanstr,    gl_deftren );
    subst_footer(resp_str);
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    free(resp_str);
    return ESP_OK;
}

static httpd_uri_t confpage = {
    .uri       = "/page_4.html",
    .method    = HTTP_GET,
    .handler   = conf_get_handler,
};

static esp_err_t conf_post_handler(httpd_req_t *req)

{
    gl_last_http = esp_timer_get_time();

    char    *buff = malloc(req->content_len + 1);
    if(!buff)
        {
        printf("no mem for conf\n");
        httpd_resp_send(req, "no mem for conf", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
        }
    int ret = httpd_req_recv(req, buff, req->content_len);
    //printf("conf req_rec ret %d\n", ret);
    (void)ret;
    buff[req->content_len] = '\0';
    //printf("conf cJSON '%s'\n", buff);
    cJSON *root5 = cJSON_Parse(buff);
    free(buff);
    char *checkedx = get_json_str(root5, "checked");
    if(checkedx[0] != '\0')
        {
        if(strcmp(checkedx, "1") == 0)
            {
            del_cache();
            //printf("Deleted hist cache\n");
            httpd_resp_send(req, "Comm history erased.", HTTPD_RESP_USE_STRLEN);
            }
        else
            {
            //printf("NOT Deleted hist cache\n");
            httpd_resp_send(req, "Must have checkbox checked.", HTTPD_RESP_USE_STRLEN);
            }
        cJSON_Delete(root5);
        return ESP_OK;
        }
    char *strxd = cJSON_GetObjectItem(root5, "deftrench")->valuestring;
    strncpy(gl_deftren,  strxd, sizeof(gl_deftren));
    submit_nvs_str("deftrench", strxd);
    cJSON_Delete(root5);
    httpd_resp_send(req, "Configuration Saved.", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static httpd_uri_t confpagep = {
    .uri       = "/page_4.html",
    .method    = HTTP_POST,
    .handler   = conf_post_handler,
};

static esp_err_t lora_post_handler(httpd_req_t *req)

{
    gl_last_http = esp_timer_get_time();

    char *resp_str;

    char    *buff = malloc(req->content_len + 1);
    if(!buff)
        {
        printf("no mem for lora post buff\n");
        httpd_resp_send(req, "no mem for lora post", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
        }
    int ret = httpd_req_recv(req, buff, req->content_len);
    buff[req->content_len] = '\0';
    (void)ret;
    //printf("lora req_rec ret %d\n", ret);
    //printf("settings lora post cJSON '%s'\n", buff);
    cJSON *root3 = cJSON_Parse(buff);
    free(buff);
    char   *tune = get_json_str(root3, "rxtune");
    if(tune[0] != '\0')
        {
        int ttt = atoi(tune);
        //printf("tune pressed %d\n", ttt);
        snprintf(gl_corrfreq, sizeof(gl_corrfreq), "%d", ttt);
        submit_nvs_str("corrfreq", gl_corrfreq);
        resp_str = "Tuning parameter set.";
        goto done;
        }
    char    *strxs, *strxb, *strxt, *strxx;
    char    *reset = get_json_str(root3, "reset");
    if(strcmp(reset, "1") == 0)
        {
        //printf("reset pressed\n");
        strxs =  DEF_SPREAD;
        strxb =  DEF_BWIDTH;
        strxt =  DEF_POWER;
        strxx =  DEF_FREQ;
        snprintf(gl_corrfreq, sizeof(gl_corrfreq), "%d", 0);
        submit_nvs_str("corrfreq", gl_corrfreq);

        resp_str = "LORA parameters reset.";
        }
    else
        {
        strxs = cJSON_GetObjectItem(root3, "spread")->valuestring;
        strxb = cJSON_GetObjectItem(root3, "bwidth")->valuestring;
        strxt = cJSON_GetObjectItem(root3, "txpower")->valuestring;
        strxx = cJSON_GetObjectItem(root3, "txfreq")->valuestring;
        resp_str = "Config Saved";
        }

    //printf("spread: %s bwidth: %s \n", strxs, strxb);
    //printf("txpow: %s txfrq: %s\n", strxt, strxx);

    strncpy(gl_spread,      strxs, sizeof(gl_spread));
    strncpy(gl_bwidth,      strxb, sizeof(gl_bwidth));
    strncpy(gl_txpower,     strxt, sizeof(gl_txpower));
    strncpy(gl_txfreq,      strxx, sizeof(gl_txfreq));

    submit_nvs_str("spread",    strxs);
    submit_nvs_str("bwidth",    strxb);
    submit_nvs_str("txpower",   strxt);
    submit_nvs_str("txfreq",    strxx);

    double  freq = atofx(gl_txfreq);
    if(freq < 1000000)
        freq *= 1000000;
    double bw = atofx(gl_bwidth);
    if(bw < 1000)
        bw *= 1000;

    //printf("freq %f\n", freq);
    //printf("bw: %f\n", bw);

    // Commit to board
    TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
    lora_set_frequency(freq);
    lora_set_bandwidth(bw);
    lora_set_tx_power(atoi(strxt));
    lora_set_spreading_factor(atoi(strxs));
    GIVE_SEMA(sSemaphore);

  done:
    cJSON_Delete(root3);
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static httpd_uri_t lorapagep = {
    .uri       = "/page_3.html",
    .method    = HTTP_POST,
    .handler   = lora_post_handler,
};

#include "h/page_2.h"

static esp_err_t settings_get_handler(httpd_req_t *req)
{
    gl_last_http = esp_timer_get_time();

    char* resp_str = xstrdup(settings_html);
    if(!resp_str)
        return ESP_FAIL;
    strcpy(resp_str, settings_html);

    char *status = "";
    subst_str(resp_str, statstr, status);
    subst_str(resp_str, netstr, gl_netname);
    subst_str(resp_str, passstr, "");
    subst_str(resp_str, pass2str, "");

    subst_str(resp_str, rebostr, "");
    subst_footer(resp_str);

    //printf("manual.html incoming query %s\n", req->uri);
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    free(resp_str);
    return ESP_OK;
}

static httpd_uri_t setpage = {
    .uri       = "/page_2.html",
    .method    = HTTP_GET,
    .handler   = settings_get_handler,
};

#include "h/channels.h"

static esp_err_t channels_get_handler(httpd_req_t *req)
{
    gl_last_http = esp_timer_get_time();

    char* resp_str = xstrdup(lora_chann_html);
    if(!resp_str)
        return ESP_FAIL;
    strcpy(resp_str, lora_chann_html);
    subst_str(resp_str, statstr, "");
    subst_str(resp_str, rebostr, "");
    subst_footer(resp_str);
    //printf("manual.html incoming query %s\n", req->uri);
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    free(resp_str);
    return ESP_OK;
}

static httpd_uri_t chann = {
    .uri       = "/channels.html",
    .method    = HTTP_GET,
    .handler   = channels_get_handler,
};

static esp_err_t netconf_post_handler(httpd_req_t *req)
{
    char    *buff = malloc(req->content_len + 1);
    if(!buff)
        {
        printf("no mem for settings post buff\n");
        httpd_resp_send(req, "no mem settins post", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
        }
    int ret = httpd_req_recv(req, buff, req->content_len);
    buff[req->content_len] = '\0';
    (void)ret;
    //printf("netconf req_rec ret %d\n", ret);
    //printf("settings post cJSON '%s'\n", buff);
    cJSON *root2 = cJSON_Parse(buff);
    free(buff);
    char *strxh = cJSON_GetObjectItem(root2, "apname")->valuestring;
    char *strxp = cJSON_GetObjectItem(root2, "appass")->valuestring;
    //printf("Host / Pass %s %s \n", strxh, strxp);
    submit_nvs_str("netname", strxh);
    submit_nvs_str("netpass", strxp);
    cJSON_Delete(root2);
    httpd_resp_send(req, "Config Saved", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static httpd_uri_t setpagepost = {
    .uri       = "/page_2.html",
    .method    = HTTP_POST,
    .handler   = netconf_post_handler,
};

#include "h/page_1.h"

int reenter = 0;

static esp_err_t live_get_handler(httpd_req_t *req)
{
    //printf("in Live handler.\n");
    if(!gl_update2)
        {
        //printf("Empty JSON sent\n");
        vTaskDelay(10 / portTICK_PERIOD_MS);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "");
        return ESP_OK;
        }
    if(reenter)
        {
        printf("Reenter\n");
        return ESP_OK;
        }
    reenter = 1;
    if(strcmp(gl_statx, "Done") == 0)
        {
        timeout_cnt = 0;
        }
    if(timeout_cnt >= 8 && timeout_cnt <= 10)
        {
        //char  tmpx[24];
        //snprintf(tmpx, sizeof(tmpx), "%s", "Timeout");
        }
    if(gl_statx[0] == '\0' || strcmp(gl_statx, "Idle ...") != 0)
        {
        timeout_cnt++;
        }
    if(gl_sentptr)
        {
        add_hist(gl_sentptr, false);
        free(gl_sentptr);
        gl_sentptr = NULL;
        }
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "strx",   gl_statx);
    cJSON_AddStringToObject(root, "ack",    gl_ack);
    cJSON_AddNumberToObject(root, "count",  timeout_cnt);
    cJSON_AddNumberToObject(root, "update", gl_update);
    cJSON_AddNumberToObject(root, "rssi",   gl_rssi);
    cJSON_AddNumberToObject(root, "devi",   gl_devi);
    cJSON_AddNumberToObject(root, "ppm",    gl_ppm);
    int sent = 0;
    if(gl_update)
        {
        for(int aa = 0; aa < gl_sentprog; aa++)
            {
            if(!gl_prom)
                {
                cJSON *root6 = cJSON_Parse(gl_sentarr[aa]);
                char *chan = cJSON_GetObjectItem(root6, "chan")->valuestring;
                int ccc = atoi(chan);
                cJSON_Delete(root6);
                //printf("chan: %d trench %d\n", ccc, gl_sendtrench);
                if(ccc != gl_sendtrench)
                    continue;
                }
            char curr[16];
            snprintf(curr, sizeof(curr), "hist_%d", sent);
            //printf("Sentarr: %d '%s' '%s'\n", gl_prom, curr, gl_sentarr[aa]);
            cJSON_AddStringToObject(root, curr, (const char*)gl_sentarr[aa]);
            sent++;
            }
        gl_update = 0;
        gl_update2 = 0;
        }
    cJSON_AddNumberToObject(root, "maxcnt", sent);

    const char *sys_info = cJSON_Print(root);
    //printf("Sending JSON: '%s'\n", sys_info);

    cJSON_Delete(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, sys_info);

    // Testing ...
    //cJSON *root4 = cJSON_Parse(sys_info);
    //char *strx2 = cJSON_GetObjectItem(root4, "strx")->valuestring;
    //printf("json strx2: '%s'\n", strx2);
    //cJSON_Delete(root4);

    free((void *)sys_info);
    //printf("Mem %ld\n", esp_get_free_heap_size());
    //printf("Integ %d\n", heap_caps_check_integrity(MALLOC_CAP_DEFAULT, true));

    reenter = 0;

    return ESP_OK;
}

static const httpd_uri_t livex = {
    .uri       = "/live.html",
    .method    = HTTP_GET,
    .handler   = live_get_handler
};

#include "h/redir.h"

static esp_err_t home_get_handler(httpd_req_t *req)
{
    gl_recala = 0;      // Clear rec LED
    gl_update = 1;
    gl_update2 = 1;

    //printf("home_get_handler()\n");

    // see if reboot requested
    //printf("home req: %s", req->uri);
    char *kkk = get_arg_ptr(req->uri);
    if (kkk)
        {
        char buff2[24], buffu2[24];
        httpd_query_key_value(kkk, "reboot", buff2, sizeof(buff2));
        unescape_url(buff2, buffu2, sizeof(buffu2));
        //printf("reboot val: '%s'\n", buffu2);
        if(strcmp(buffu2, "true") == 0)
            {
            char* resp_str = xstrdup(redir_html);
            if(!resp_str)
                {
                httpd_resp_send(req, "No Mem for redir", HTTPD_RESP_USE_STRLEN);
                return ESP_OK;
                }
            subst_footer(resp_str);
            httpd_resp_set_type(req, "text/html");
            httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
            free(resp_str);
            delayed_reboot(500);
            return ESP_OK;
            }
        }
    char* resp_str = xstrdup(index_html);
    if(!resp_str)
        {
        httpd_resp_send(req, "No Mem for home", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
        }
    subst_str(resp_str, trenchstr, gl_deftren);
    subst_footer(resp_str);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    free(resp_str);
    return ESP_OK;
}

static const httpd_uri_t hindex = {
    .uri       = "/page_1.html",
    .method    = HTTP_GET,
    .handler   = home_get_handler
};

static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = home_get_handler
};

char iconx[] =
"<link href=\"data:image/x-icon;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQEAYAAABPYyMiAAAABmJLR0T///////8JWPfcAAAACXBIWXMAAABIAAAASABGyWs+AAAAF0lEQVRIx2NgGAWjYBSMglEwCkbBSAcACBAAAeaR9cIAAAAASUVORK5CYII=\" rel=\"icon\" type=\"image/x-icon\" />";

static esp_err_t root_icon_handler(httpd_req_t *req)
{
    //printf("Icon handler. '%s'\n", req->uri);
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, iconx, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t faviconx = {
    .uri       = "favicon.ico",
    .method    = HTTP_GET,
    .handler   = root_icon_handler
};

static esp_err_t home_post_handler(httpd_req_t *req)
{
    gl_recala = 0;      // Clear rec LED
    gl_update2 = 1;
    gl_update = 1;

    //printf("home_post_handler()\n");

    //char* resp_str = xstrdup(index_html);
    //if(!resp_str)
    //    {
    //    httpd_resp_send(req, "No Mem for home", HTTPD_RESP_USE_STRLEN);
    //    return ESP_OK;
    //    }
    //
    httpd_resp_set_type(req, "text/html");
    //httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send(req, "POST", HTTPD_RESP_USE_STRLEN);
    //free(resp_str);
    return ESP_OK;
}

static const httpd_uri_t rootp = {
    .uri       = "/",
    .method    = HTTP_POST,
    .handler   = home_post_handler
};

static const httpd_uri_t sindex = {
    .uri       = "/page_1.html",
    .method    = HTTP_POST,
    .handler   = home_post_handler
};

int reent = 0;

static esp_err_t post_post_handler(httpd_req_t *req)
{
    gl_recala = 0;      // Clear rec LED

    if(reent)
        {
        printf("POST reentry\n");
        }
    reent = 1;
    //printf("post_post_handler()");
    printf("post_post content_len %d\n", req->content_len);
    char    *buff = malloc(req->content_len + 1);
    if(!buff)
        {
        printf("no mem for post buff\n");
        httpd_resp_send(req, "no mem for post buf", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
        }
    vTaskDelay(10 / portTICK_PERIOD_MS);

    int ret = httpd_req_recv(req, buff, req->content_len);
    //printf("post req_rec ret %d\n", ret);
    buff[req->content_len] = '\0';
    (void)ret;
    //printf("JSON buff'%s'\n", buff);
    cJSON *root2 = cJSON_Parse(buff);
    free(buff);
    char   *trenchs = get_json_str(root2, "chan");
    int     check   = get_json_int(root2, "check");
    if(check != -1)
        {
        gl_prom = check;
        gl_sendtrench = atoi(trenchs);
        cJSON_Delete(root2);
        //printf("Check prom: %d gl_sendtrench: %d\n", gl_prom, gl_sendtrench);
        gl_update = 1;
        gl_update2 = 1;
        httpd_resp_send(req, "Check ...", HTTPD_RESP_USE_STRLEN);
        reent = 0;
        return ESP_OK;
        }
    char *strx2   = cJSON_GetObjectItem(root2, "text")->valuestring;
    // Check Chash
    //printf("json strx2: '%s'\n", strx2);
    //int16_t sss = chksum(strx2, strlen(strx2));
    //printf("Chash=%x\n", sss & 0xffff);
    // Pad string with <br>
    xStr *yyy = xstr_create(0);
    xstr_padbr(yyy, strx2, "<br>", 70);
    cJSON_AddStringToObject(root2, "text", yyy->str);
    xstr_destroy(yyy);
    cJSON_AddStringToObject(root2, "reply", "");
    char *buff2 = cJSON_Print(root2);
    if(!buff2)
        {
        printf("no mem for buff2\n");
        httpd_resp_send(req, "no mem for buff2", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
        }
    gl_sentptr  = buff2;
    gl_sentbuff = xstrdup(strx2);
    gl_sendtrench = atoi(trenchs);
    gl_update = 1;
    gl_update2 = 1;
    cJSON_Delete(root2);
    httpd_resp_send(req, "Submitted ...", HTTPD_RESP_USE_STRLEN);
    //printf("Mem %ld\n", esp_get_free_heap_size());
    //printf("Integ %d\n", heap_caps_check_integrity(MALLOC_CAP_DEFAULT, true));
    reent = 0;
    GIVE_SEMA(iSemaphore);
    return ESP_OK;
}

static const httpd_uri_t postxpostx = {
    .uri       = "/post.html",
    .method    = HTTP_POST,
    .handler   = post_post_handler
};

void stop_webservers(void)
{
    // Stop the httpd servers
    httpd_ssl_stop(gl_serverx);
    httpd_stop(gl_server);
}

void start_webservers(void)
{
    //httpd_handle_t server = NULL;
    // Start the httpd server
    //ESP_LOGI(TAG, "Starting servers");

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
    conf.httpd.max_uri_handlers = 20;
    conf.httpd.max_open_sockets = 2;
    conf.httpd.lru_purge_enable = true;

    extern const unsigned char servercert_start[] asm("_binary_espcert_pem_start");
    extern const unsigned char servercert_end[]   asm("_binary_espcert_pem_end");
    conf.servercert = servercert_start;
    conf.servercert_len = servercert_end - servercert_start;

    extern const unsigned char prvtkey_pem_start[] asm("_binary_espkey_pem_start");
    extern const unsigned char prvtkey_pem_end[]   asm("_binary_espkey_pem_end");
    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

#if CONFIG_EXAMPLE_ENABLE_HTTPS_USER_CALLBACK
    conf.user_cb = https_server_user_callback;
#endif
    esp_err_t ret = httpd_ssl_start(&gl_serverx, &conf);
    if (ESP_OK != ret) {
        ESP_LOGI(TAG, "Error starting server!");
        return; // NULL;
    }

    // Generate default configuration
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 20;
    config.max_open_sockets = 2;
    config.lru_purge_enable = true;

    // Start the plain httpd server
    if (httpd_start(&gl_server, &config) != ESP_OK) {
        printf("Cannot start web server.");
        }

    // Set URI handlers
    //ESP_LOGI(TAG, "Registering URI handlers");

    httpd_register_uri(&livex);
    httpd_register_uri(&root);
    httpd_register_uri(&rootp);
    httpd_register_uri(&hindex);
    httpd_register_uri(&sindex);
    httpd_register_uri(&faviconx);
    httpd_register_uri(&chann);
    httpd_register_uri(&setpage);
    httpd_register_uri(&setpagepost);
    httpd_register_uri(&manpage);
    httpd_register_uri(&lorapage);
    httpd_register_uri(&lorapagep);
    httpd_register_uri(&confpage);
    httpd_register_uri(&confpagep);
    httpd_register_uri(&postxpostx);
}

// EOF
