idf_component_register(
    SRCS
        "src/epd_panel.c"
    REQUIRES
        driver
        esp_timer
    INCLUDE_DIRS
        "include")