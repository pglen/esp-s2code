cmake_minimum_required(VERSION 3.13)

idf_component_register(SRCS "recv.c"
                    INCLUDE_DIRS "."
                    REQUIRES lora led_strip console
                    PRIV_REQUIRES esp_timer
                    )
