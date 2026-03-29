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

// Shuffle a string to 16 bit unique ID
//int16_t chksum(const char *str, int len)
//{
//    //printf("str '%s'\n", str);
//    uint16_t ret = 0;
//    for(int aa = 0; aa < len; aa++)
//        {
//        uint16_t nn = (uint16_t)str[aa];
//        uint16_t qq = nn << 7 | nn;
//        ret += qq + 10000;
//        ret ^= 0x5aa5;
//        }
//    //printf("sum ret %x\n", ret);
//    return(ret);
//}

#define VT_SAVECURSOR            "\e7"  /* Save cursor and attrib */
#define VT_RESTORECURSOR         "\e8"  /* Restore cursor pos and attribs */
#define VT_SAVECURSOR2           "\es"  /* Save cursor and attrib */
#define VT_RESTORECURSOR2        "\eu"  /* Restore cursor pos and attribs */
#define VT_SETWIN_CLEAR          "\e[r" /* Clear scrollable window size */
#define VT_CLEAR_SCREEN          "\e[2J" /* Clear screen */
#define VT_CLEAR_LINE            "\e[2K" /* Clear this whole line */
#define VT_RESET_TERMINAL        "\ec"


void    out_str(const char *strx, const char *str2)

{
    printf(VT_SAVECURSOR);
    printf("\033[1;12r");
    printf("\033[12;1H");
    printf("%s", strx);
    printf("%s", str2);
    //printf("\033[1;24r");
    printf("\033[13;24r");
    //printf("\033[18;1H");
    printf(VT_RESTORECURSOR);
}

int splity = 12;

void    set_split()
{
    printf("\033[%d;24r", splity);
}

