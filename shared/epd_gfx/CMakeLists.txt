set(EPD_GFX_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPD_GFX_SRCS
    canvas.c
    codec.c)

if(COMMAND idf_component_register)
    idf_component_register(
        REQUIRES
            epd_core
        INCLUDE_DIRS
            ${EPD_GFX_INCLUDE_DIR}
        SRCS
            ${EPD_GFX_SRCS})
else()
    add_library(epd_gfx STATIC ${EPD_GFX_SRCS})
    target_include_directories(epd_gfx PUBLIC $<BUILD_INTERFACE:${EPD_GFX_INCLUDE_DIR}>)
    target_link_libraries(epd_gfx PUBLIC epd_core)
    target_compile_features(epd_gfx PUBLIC c_std_99)
endif()
