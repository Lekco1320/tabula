idf_component_register(
    SRCS
        epd_vfs.c
    REQUIRES
        epd_core
        spiffs
    INCLUDE_DIRS
        include)
