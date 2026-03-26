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

