
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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include <esp_timer.h>
#include <esp_https_server.h>
#include <esp_http_server.h>
#include <esp_tls.h>

#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"

#include "cJSON.h"

#include "../common/protocol.h"
#include "../common/utils.h"
#include "../common/common.h"
#include "../common/wifi.h"
#include "../common/nvs.h"
#include "../common/strlib.h"
#include "../common/comline.h"
#include "../common/packets.h"
#include "../common/comline.h"

#include "lora.h"
#include "leds.h"

static const char *TAG = "lorawifi";

char    *gl_sentptr = NULL;
char    *gl_sentbuff = NULL;
int     gl_update = 0;

#define PROG_VER    "1.0"
#define PROG_DATE   "Mon 23.Mar.2026"


uint64_t    gl_last_http = 0;

static SemaphoreHandle_t iSemaphore  = NULL;
static SemaphoreHandle_t hSemaphore  = NULL;

// This protects the LORA subsystem
SemaphoreHandle_t sSemaphore  = NULL;

char      gl_statx[64] = "";

#define RET_IFZERO(valx, retval) if ((valx)) == 0) return((retval));

#if CONFIG_ESP_GTK_REKEYING_ENABLE
#define EXAMPLE_GTK_REKEY_INTERVAL CONFIG_ESP_GTK_REKEY_INTERVAL
#else
#define EXAMPLE_GTK_REKEY_INTERVAL 0
#endif


static httpd_handle_t gl_serverx = NULL;
static httpd_handle_t gl_server = NULL;

#define SENTMAX 24        // Maximum history items

const   char *sentarr[SENTMAX] = {0, };

int     gl_recala       = 0;
int     gl_sentprog     = 0;

char    gl_spread[32]   = "";
char    gl_bwidth[32]   = "";
char    gl_txpower[32]  = "";
char    gl_txfreq[32]   = "";
char    gl_deftren[32]  = "";

char    gl_curr_tr[32] = "0";

//include "../common/packets.c"

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

// Read var, if none, set default

static void read_set_def(nvs_handle handle, char *key, char *val, int maxlen, char *defval)
{
    unsigned int olen = maxlen;
    esp_err_t err = nvs_get_str(handle, key, val, &olen);
    if (err != ESP_OK || val[0] == '\0')
        {
        //ESP_LOGI(TAG, "Warn (%d) reading NVS %s", err, key, defval);
        strncpy(val, defval, maxlen);
        err = nvs_set_str(handle, key, defval);
        if (err != ESP_OK)
            ESP_LOGI(TAG, "ERR (%d) writing default NVS %s", err, key, defval);
        printf("nvs def: %s -> '%s'\n", key, val);
        }
    else
        {
        printf("nvs read: %s -> '%s'\n", key, val);
        }
}

// Read / set default values
void    read_nvs_vars()
{
    char netname[64];
    nvs_handle my_handle;

    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        {
        printf("Error (%d) opening NVS handle.", err);
        return;
        }

    GET_MAC(self_mac);
    snprintf(netname, sizeof(netname),
                "LoraWiFi-%02X%02X", self_mac[4], self_mac[5]);

    read_set_def(my_handle, "netname", gl_netname, sizeof(gl_netname), netname);
    read_set_def(my_handle, "netpass", gl_netpass, sizeof(gl_netpass), "12345678");

    read_set_def(my_handle, "spread",    gl_spread,    sizeof(gl_spread),   DEF_SPREAD);
    read_set_def(my_handle, "bwidth",    gl_bwidth,    sizeof(gl_bwidth),   DEF_BWIDTH);
    read_set_def(my_handle, "txpower",   gl_txpower,   sizeof(gl_txpower),  DEF_POWER);
    read_set_def(my_handle, "txfreq",    gl_txfreq,    sizeof(gl_txfreq),   DEF_FREQ);
    read_set_def(my_handle, "deftrench", gl_deftren, sizeof(gl_deftren),DEF_TRENCH);

    nvs_close(my_handle);
}

#if CONFIG_EXAMPLE_ENABLE_HTTPS_USER_CALLBACK
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
static void print_peer_cert_info(const mbedtls_ssl_context *ssl)
{
    const mbedtls_x509_crt *cert;
    const size_t buf_size = 1024;
    char *buf = calloc(buf_size, sizeof(char));
    if (buf == NULL) {
        ESP_LOGE(TAG, "Out of memory - Callback execution failed!");
        return;
    }

    // Logging the peer certificate info
    cert = mbedtls_ssl_get_peer_cert(ssl);
    if (cert != NULL) {
        mbedtls_x509_crt_info((char *) buf, buf_size - 1, "    ", cert);
        ESP_LOGI(TAG, "Peer certificate info:\n%s", buf);
    } else {
        ESP_LOGW(TAG, "Could not obtain the peer certificate!");
    }

    free(buf);
}
#endif

static void https_server_user_callback(esp_https_server_user_cb_arg_t *user_cb)
{
    ESP_LOGI(TAG, "User callback invoked!");
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
    mbedtls_ssl_context *ssl_ctx = NULL;
#endif
    switch(user_cb->user_cb_state) {
        case HTTPD_SSL_USER_CB_SESS_CREATE:
            ESP_LOGD(TAG, "At session creation");

            // Logging the socket FD
            int sockfd = -1;
            esp_err_t esp_ret;
            esp_ret = esp_tls_get_conn_sockfd(user_cb->tls, &sockfd);
            if (esp_ret != ESP_OK) {
                ESP_LOGE(TAG, "Error in obtaining the sockfd from tls context");
                break;
            }
            ESP_LOGI(TAG, "Socket FD: %d", sockfd);
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
            ssl_ctx = (mbedtls_ssl_context *) esp_tls_get_ssl_context(user_cb->tls);
            if (ssl_ctx == NULL) {
                ESP_LOGE(TAG, "Error in obtaining ssl context");
                break;
            }
            // Logging the current ciphersuite
            ESP_LOGI(TAG, "Current Ciphersuite: %s", mbedtls_ssl_get_ciphersuite(ssl_ctx));
#endif
            break;

        case HTTPD_SSL_USER_CB_SESS_CLOSE:
            ESP_LOGD(TAG, "At session close");
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
            // Logging the peer certificate
            ssl_ctx = (mbedtls_ssl_context *) esp_tls_get_ssl_context(user_cb->tls);
            if (ssl_ctx == NULL) {
                ESP_LOGE(TAG, "Error in obtaining ssl context");
                break;
            }
            print_peer_cert_info(ssl_ctx);
#endif
            break;
        default:
            ESP_LOGE(TAG, "Illegal state!");
            return;
    }
}
#endif

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
    char vtmp[48];
    snprintf(vtmp, sizeof(vtmp), "%s %s", PROG_VER, PROG_DATE);
    subst_str(mem, softstr, vtmp);
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
    subst_str(resp_str, statstr,    "");
    subst_str(resp_str, spreadstr,  gl_spread    );
    subst_str(resp_str, bandstr,    gl_bwidth    );
    subst_str(resp_str, txpowstr,   gl_txpower   );
    subst_str(resp_str, freqstr,    gl_txfreq    );
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
    httpd_req_recv(req, buff, req->content_len);
    buff[req->content_len] = '\0';
    //printf("conf cJSON '%s'\n", buff);
    cJSON *root5 = cJSON_Parse(buff);
    free(buff);
    char *strxd = cJSON_GetObjectItem(root5, "deftrench")->valuestring;
    strncpy(gl_deftren,  strxd, sizeof(gl_deftren));
    submit_nvs_str("deftrench", strxd);
    cJSON_Delete(root5);
    httpd_resp_send(req, "Configuration Saved", HTTPD_RESP_USE_STRLEN);
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

    char    *buff = malloc(req->content_len + 1);
    if(!buff)
        {
        printf("no mem for lora post buff\n");
        httpd_resp_send(req, "no mem for lora post", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
        }
    httpd_req_recv(req, buff, req->content_len);
    buff[req->content_len] = '\0';
    //printf("settings lora post cJSON '%s'\n", buff);
    cJSON *root3 = cJSON_Parse(buff);
    free(buff);

    char *strxs = cJSON_GetObjectItem(root3, "spread")->valuestring;
    char *strxb = cJSON_GetObjectItem(root3, "bwidth")->valuestring;
    char *strxt = cJSON_GetObjectItem(root3, "txpower")->valuestring;
    char *strxx = cJSON_GetObjectItem(root3, "txfreq")->valuestring;

    printf("Spread: %s bwidth: %s \n", strxs, strxb);
    printf("Txpow: %s txfrq: %s\n", strxt, strxx);

    strncpy(gl_spread,      strxs, sizeof(gl_spread));
    strncpy(gl_bwidth,      strxb, sizeof(gl_bwidth));
    strncpy(gl_txpower,     strxt, sizeof(gl_txpower));
    strncpy(gl_txfreq,      strxx, sizeof(gl_txfreq));

    submit_nvs_str("spread",    strxs);
    submit_nvs_str("bwidth",    strxb);
    submit_nvs_str("txpower",   strxt);
    submit_nvs_str("txfreq",    strxx);

    // Commit to board
    double  freq = atofx(strxx);
    if(freq < 1000000)
        freq *= 1000000;
    double  bw = atofx(strxb);
    if(bw < 1000)
        bw *= 1000;

    printf("freq %f\n", freq);
    printf("bw %f\n", bw);
    printf("pow: %d\n", atoi(strxt));
    printf("spread: %d\n", atoi(strxs));

    TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
    lora_set_frequency(freq);
    lora_set_bandwidth(bw);
    lora_set_tx_power(atoi(strxt));
    lora_set_spreading_factor(atoi(strxs));
    GIVE_SEMA(sSemaphore);

    cJSON_Delete(root3);

    //printf("manual.html incoming query %s\n", req->uri);
    char *resp_str = "Config Saved";
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
    httpd_req_recv(req, buff, req->content_len);
    buff[req->content_len] = '\0';
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

static  int timeout_cnt = 0;

char  tmpx[96];

static  void    add_hist(const char *item)

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
        free((void*)sentarr[0]);
        // Shift down
        for(int aa = 1; aa < gl_sentprog; aa++)
            {
            sentarr[aa-1] = sentarr[aa];
            }
        gl_sentprog--;
        }
    sentarr[gl_sentprog] = tmp;
    gl_sentprog++;

    GIVE_SEMA(hSemaphore);
}

static esp_err_t live_get_handler(httpd_req_t *req)
{
    //printf("in Live handler.\n");

    if(strcmp(gl_statx, "Done") == 0)
        {
        timeout_cnt = 0;
        }
    //if(timeout_cnt > 9 && timeout_cnt < 11)
    //    {
    //    gl_statx[0] = '\0';
    //    }
    if(timeout_cnt >= 8 && timeout_cnt <= 10)
        {
        snprintf(tmpx, sizeof(tmpx), "%s", "Timeout");
        }
    if(gl_statx[0] == '\0' || strcmp(gl_statx, "Idle ...") != 0)
        {
        timeout_cnt++;
        }
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "strx", gl_statx);
    cJSON_AddNumberToObject(root, "count", timeout_cnt);
    cJSON_AddNumberToObject(root, "update", gl_update);
    cJSON_AddNumberToObject(root, "maxcnt", gl_sentprog);
    if(gl_update)
        {
        gl_update = 0;
        for(int aa = 0; aa < gl_sentprog; aa++)
            {
            char curr[16];
            snprintf(curr, sizeof(curr), "hist_%d", aa);
            //printf("Sentarr: '%s' '%s'\n", curr, sentarr[aa]);
            cJSON_AddStringToObject(root, curr, (const char*)sentarr[aa]);
            }
        if(gl_sentptr)
            {
            add_hist(gl_sentptr);
            free(gl_sentptr);
            gl_sentptr = NULL;
            }
        }
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

    return ESP_OK;
}

static esp_err_t home_get_handler(httpd_req_t *req)
{
    char* resp_str = xstrdup(index_html);
    if(!resp_str)
        {
        httpd_resp_send(req, "No Mem for home", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
        }
    // see if reboot requested
    //printf("home req: %s", req->uri);
    char *kkk = get_arg_ptr(req->uri);
    if (kkk)
        {
        char buff2[24], buffu2[24];
        httpd_query_key_value(kkk, "reboot", buff2, sizeof(buff2));
        unescape_url(buff2, buffu2, sizeof(buffu2));
        printf("reboot val: '%s'\n", buffu2);
        if(strcmp(buffu2, "true") == 0)
            {
            delayed_reboot(500);
            }
        }
    subst_str(resp_str, trenchstr, gl_deftren);
    subst_footer(resp_str);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    free(resp_str);
    return ESP_OK;
}

char iconx[] =
"<link href=\"data:image/x-icon;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQEAYAAABPYyMiAAAABmJLR0T///////8JWPfcAAAACXBIWXMAAABIAAAASABGyWs+AAAAF0lEQVRIx2NgGAWjYBSMglEwCkbBSAcACBAAAeaR9cIAAAAASUVORK5CYII=\" rel=\"icon\" type=\"image/x-icon\" />";

static esp_err_t root_icon_handler(httpd_req_t *req)
{
    printf("Icon handler. '%s'\n", req->uri);
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, iconx, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t home_post_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send(req, "POST", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t post_post_handler(httpd_req_t *req)
{
    //printf("post_post content_len %d\n", req->content_len);
    char    *buff = malloc(req->content_len + 1);
    if(!buff)
        {
        printf("no mem for buff\n");
        httpd_resp_send(req, "no mem", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
        }
    httpd_req_recv(req, buff, req->content_len);
    buff[req->content_len] = '\0';
    //printf("JSON buff'%s'\n", buff);
    cJSON *root2 = cJSON_Parse(buff);
    free(buff);
    char *strx2 = cJSON_GetObjectItem(root2, "text")->valuestring;
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

    cJSON_Delete(root2);
    httpd_resp_send(req, "Submitted ...", HTTPD_RESP_USE_STRLEN);
    //printf("Mem %ld\n", esp_get_free_heap_size());
    //printf("Integ %d\n", heap_caps_check_integrity(MALLOC_CAP_DEFAULT, true));
    GIVE_SEMA(iSemaphore);
    return ESP_OK;
}

static const httpd_uri_t livex = {
    .uri       = "/live.html",
    .method    = HTTP_GET,
    .handler   = live_get_handler
};

static const httpd_uri_t postxpostx = {
    .uri       = "/post.html",
    .method    = HTTP_POST,
    .handler   = post_post_handler
};

static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = home_get_handler
};

static const httpd_uri_t rootp = {
    .uri       = "/",
    .method    = HTTP_POST,
    .handler   = home_post_handler
};

static const httpd_uri_t faviconx = {
    .uri       = "favicon.ico",
    .method    = HTTP_GET,
    .handler   = root_icon_handler
};

static const httpd_uri_t hindex = {
    .uri       = "/page_1.html",
    .method    = HTTP_GET,
    .handler   = home_get_handler
};

static const httpd_uri_t sindex = {
    .uri       = "/page_1.html",
    .method    = HTTP_POST,
    .handler   = home_post_handler
};

static void start_webservers(void)
{
    //httpd_handle_t server = NULL;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting servers");

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

static void stop_webservers()
{
    // Stop the httpd servers
    httpd_ssl_stop(gl_serverx);
    httpd_stop(gl_server);
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
        start_webservers();

    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
        stop_webservers();
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
             //.password = CONFIG_EXAMPLE_WIFI_PASSWORD,
             //.ssid = CONFIG_EXAMPLE_WIFI_SSID,
             //.ssid_len = strlen(CONFIG_EXAMPLE_WIFI_SSID),
            .channel = CONFIG_ESP_WIFI_CHANNEL,
            .max_connection = CONFIG_ESP_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                    .required = true,
            },
#ifdef CONFIG_ESP_WIFI_BSS_MAX_IDLE_SUPPORT
            .bss_max_idle_cfg = {
                .period = WIFI_AP_DEFAULT_MAX_IDLE_PERIOD,
                .protected_keep_alive = 1,
            },
#endif
            .gtk_rekey_interval = EXAMPLE_GTK_REKEY_INTERVAL,
        },
    };
    if (strlen(CONFIG_EXAMPLE_WIFI_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    read_nvs_vars();

    memcpy(wifi_config.ap.ssid, gl_netname, sizeof(gl_netname));
    memcpy(wifi_config.ap.password, gl_netpass, sizeof(gl_netpass));

    //printf("Wifi Name '%s' -> '%s'\n",
    //                wifi_config.ap.ssid, wifi_config.ap.password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    //ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
    //         EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s channel:%d",
             CONFIG_EXAMPLE_WIFI_SSID, CONFIG_ESP_WIFI_CHANNEL);
}

char *testx =   "Simulated Recv Simulated Recv Simulated Recv "
                "Simulated Recv Simulated Recv Simulated Recv "
                "Simulated Recv Simulated Recv Simulated Recv "
                "Simulated %d";

char    gl_buffer[256];
char    gl_buff2[256];

static  void recv_task (void* arg)
{
    int cntR = 0;
    char  cstr[16], hstr[16];

    for(;;) {

        TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
        lora_receive();
        gl_buffer[0] = '\0';
        int reclen = lora_receive_packet((uint8_t*)gl_buffer, sizeof(gl_buffer));
        GIVE_SEMA(sSemaphore);

        if (reclen == 0)
            {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
            }
        gl_buffer[reclen] = '\0';
        int isOK = check_packet(gl_buffer, reclen);
        if(!isOK)
            {
            printf("Error on packet: '%s'\n", gl_buffer);
            }
        toggle_led(isOK);

        TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
        int rssi = lora_packet_rssi();
        int ret = lora_read_freq_err();
        GIVE_SEMA(sSemaphore);
        print_freq_deviation(ret);
        char buff2[64];
        snprintf(buff2, sizeof(buff2) - 1,
                "Recvd: %d len: %d paylen: %d rssi: %d cnt: %d check: %d",
                    cntR, reclen, reclen - 5, rssi, cntR, isOK);
        printf("%s\n", buff2);
        cntR++;

        //printf("gl_buffer '%s'\n", &gl_buffer[5]);
        //xStr *sss = xstr_dumpbuff( &gl_buffer[5], strlen( &gl_buffer[5]) + 4);
        //printf("'%s'\n", sss->str);
        //xstr_destroy(sss);

        snprintf(cstr, sizeof(cstr), "%d", atoi(gl_curr_tr));
        xStr *sss = xstr_create(0);
        xstr_padbr(sss, &gl_buffer[5], "<br>", 70);

        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "reply", sss->str);
        int16_t sumx = lora_chksum(sss->str, sss->length);
        xstr_destroy(sss);
        snprintf(hstr, sizeof(hstr), "%x", sumx & 0xffff);
        //printf("Chash %s\n", shstr);
        //printf("cstr: '%s' hstr: '%s'\n", cstr, hstr);
        cJSON_AddStringToObject(root, "chan", cstr);
        cJSON_AddStringToObject(root, "hash", hstr);
        cJSON_AddNumberToObject(root, "isok", isOK);
        char *buff3 = cJSON_Print(root);
        cJSON_Delete(root);

        add_hist(buff3);
        free(buff3);

        gl_update = 1;
        gl_recala = 1;

        GIVE_SEMA(hSemaphore);

        //snprintf(gl_statx, sizeof(gl_statx), "%s", "Done");
        //vTaskDelay(1000 / portTICK_PERIOD_MS);
        //snprintf(gl_statx, sizeof(gl_statx), "%s", "Idle ...");
        //vTaskDelay(1000 / portTICK_PERIOD_MS);
        //printf("Sent idle.\n");

        vTaskDelay(pdMS_TO_TICKS(100));
        }
}

#if 0
static  void recv_sim_task (void* arg)
{
    uint32_t vnt = 1, ccc = esp_random() % 0x8000;
    char  cstr[16], hstr[16];

    while(true)
        {
        xStr  *ttt = xstr_sprintf(testx, vnt++);
        if(!ttt->str)
            {
            printf("No mem for recv\n");
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
            }
        //printf("ttt: '%s'\n", ttt->str);
        snprintf(cstr, sizeof(cstr), "%ld", ccc);
        xStr *sss = xstr_create(0);
        xstr_padbr(sss, ttt->str, "<br>", 70);
        xstr_destroy(ttt);
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "reply", sss->str);
        int16_t sumx = lora_chksum(sss->str, sss->length);
        xstr_destroy(sss);
        //printf("Chash %x\n", sumx & 0xffff);
        snprintf(hstr, sizeof(hstr), "%x", sumx & 0xffff);
        //printf("cstr: '%s' hstr: '%s'\n", cstr, hstr);
        cJSON_AddStringToObject(root, "chan", cstr);
        cJSON_AddStringToObject(root, "hash", hstr);
        char *buff3 = cJSON_Print(root);
        cJSON_Delete(root);
        if(!buff3)
            {
            printf("No mem for buff3\n");
            continue;
            }
        else
            {
            add_hist(buff3);
            free(buff3);
            }
        gl_recala = 1;
        gl_update = 1;
        //printf("Mem recv %ld\n", esp_get_free_heap_size());
        //printf("Integ %d\n", heap_caps_check_integrity(MALLOC_CAP_DEFAULT, true));
        //heap_caps_print_single_task_stat(NULL, NULL);

        //uint32_t rrr = esp_random() % 4000;
        //vTaskDelay(rrr / portTICK_PERIOD_MS);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
}
#endif

static  void trans_task (void* arg)
{
    TAKE_SEMA(sSemaphore, TAG, portMAX_DELAY);
    lora_hw_init(); // Initialize the LoRa module (pins configured in menuconfig)
    init_lora_common();     // Send / recv common config
    GIVE_SEMA(sSemaphore);

    while(true)
        {
        //printf("Before semaphore release cycle.\n");
        TAKE_SEMA(iSemaphore, TAG, portMAX_DELAY);
        //printf("%lld ms - After semaphore release cycle.\n",
        //                    esp_timer_get_time()/1000);
        //vTaskDelay(1000 / portTICK_PERIOD_MS);
        rr = 100, gg = 50, bb = 0;
        blink_cnt = 1;
        snprintf(gl_statx, sizeof(gl_statx), "%s", "Sending ...");
        send_payload(gl_sentbuff);
        free(gl_sentbuff);
        gl_update = 1;
        snprintf(gl_statx, sizeof(gl_statx), "%s", "Done");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        snprintf(gl_statx, sizeof(gl_statx), "%s", "Idle ...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        timeout_cnt = 0;

        //vTaskDelay(2000 / portTICK_PERIOD_MS);
        //snprintf(gl_statx, sizeof(gl_statx), "%s", "");
        //printf("Done sema.\n");
        //pulse_led(80, 80, 40, 00);
        blink_cnt = 2;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        }
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
        }
    ESP_ERROR_CHECK(ret);

    // See if button held, clear flash
    gpio_num_t gpio_num = 1;
    gpio_input_enable(gpio_num);
    gpio_pullup_en(gpio_num);
    int lev = gpio_get_level(gpio_num);
    //printf("GPIO level %d %d\n", gpio_num, lev);
    if(lev == 0)
        {
        printf("Clearing nvram ...\n");
        //nvs_flash_erase();
        //ret = nvs_flash_init();
        nvs_handle my_handle;
        esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
        if (err == ESP_OK)
            {
            nvs_erase_key(my_handle, "netname");
            nvs_erase_key(my_handle, "netpass");
            nvs_erase_key(my_handle, "spread");
            nvs_erase_key(my_handle, "bwidth");
            nvs_erase_key(my_handle, "txpower");
            nvs_erase_key(my_handle, "txfreq");
            nvs_erase_key(my_handle, "deftrench");

            nvs_close(my_handle);
            }
        }
    //ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
    printf("Entering main loop ... mem = %ld\n", esp_get_free_heap_size());

    configure_led();

    CREATE_SEMA(iSemaphore); TAKE_SEMA(iSemaphore, TAG, portMAX_DELAY);
    CREATE_SEMA(hSemaphore); CREATE_SEMA(sSemaphore);

    start_console("LoraWifi>");

    xTaskCreate(&trans_task, "trans_task", 3048, NULL, 15, NULL);
    xTaskCreate(&recv_task, "recv_task", 3048, NULL, 15, NULL);
    //xTaskCreate(&recv_sim_task, "recv_sim_task", 3048, NULL, 15, NULL);
    //get_nvs_info();

    rr = 0, gg = 0, bb = 50;
    blink_cnt = 3;
    //pulse_led(40, 0, 0, 55);

    int cnt = 0;
    while(1)
        {
        //uint32_t ttt = esp_timer_get_time()/1000;
        //printf("time: %ld\n", ttt);
        //if(cnt % 12 == 0)
        //    printf("Integ %d\n", heap_caps_check_integrity(MALLOC_CAP_DEFAULT, true));
        //toggle_led(0);

        if(cnt % 4 == 0)
            {
            if(verbose)
                printf("Mem main: %ld bytes\n", esp_get_free_heap_size());
            if(gl_recala)
                {
                //pulse_led(40, 0, 55, 0);
                rr = 0, gg = 50, bb = 0;
                blink_cnt = 2;
                }
            }
        cnt++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
}

// EOF
