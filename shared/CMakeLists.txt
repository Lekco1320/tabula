if(NOT COMMAND idf_component_register)
    add_subdirectory(epd_core)
    add_subdirectory(epd_gfx)
endif()
