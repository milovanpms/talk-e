idf_component_register(
    SRCS "i2c_oled_example_main.c" "lvgl_demo_ui.c"
    INCLUDE_DIRS "."
    REQUIRES esp_lcd esp_driver_i2c esp_timer lvgl
)
