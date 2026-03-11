cmake_minimum_required(VERSION 3.13)
idf_component_register(SRCS "lora.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "esp_driver_spi" "esp_driver_gpio" )
