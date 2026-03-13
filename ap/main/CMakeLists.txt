idf_component_register(SRCS main.c
                    ../common/utils.c   ../common/sere.c
                    ../common/protocol.c ../common/wifi.c
                    ../common/nvs.c
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES json esp_wifi nvs_flash esp_https_server
                                    esp_timer esp_driver_gpio
                    EMBED_TXTFILES "certs/espcert.pem"
                               "certs/espkey.pem"
                    )
