idf_component_register(SRCS main.c
                    ../common/utils.c  ../common/protocol.c
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES json esp_wifi nvs_flash esp_https_server esp_timer
                    EMBED_TXTFILES "certs/espcert.pem"
                               "certs/espkey.pem"
                    )
