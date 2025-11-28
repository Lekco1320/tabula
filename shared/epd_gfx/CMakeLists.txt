set(EPD_GFX_INCLUDE_DIR include)

if(COMMAND idf_component_register)
    idf_component_register(
        REQUIRES
            epd_core
        INCLUDE_DIRS
            ${EPD_GFX_INCLUDE_DIR})
else()
    add_library(epd_gfx INTERFACE)
    target_include_directories(epd_gfx PUBLIC $<BUILD_INTERFACE:${EPD_GFX_INCLUDE_DIR}>)
    target_compile_features(epd_gfx PUBLIC c_std_99)
endif()