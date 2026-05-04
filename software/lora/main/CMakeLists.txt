idf_component_register(SRCS "main.c"
                            "cmd_i2ctools.c"
                            "oled.c"
                       PRIV_REQUIRES esp_driver_gpio esp_driver_spi esp_driver_i2c console lora u8g2
                       INCLUDE_DIRS ".")
