idf_component_register(SRCS "lora.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_driver_spi esp_driver_gpio esp_system soc driver)
