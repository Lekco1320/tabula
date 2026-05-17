set(EPD_CORE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPD_CORE_SRCS
    common.c
    stream.c)

if(COMMAND idf_component_register)
    idf_component_register(
        INCLUDE_DIRS
            ${EPD_CORE_INCLUDE_DIR}
        SRCS
            ${EPD_CORE_SRCS})
else()
    add_library(epd_core STATIC ${EPD_CORE_SRCS})
    set_target_properties(epd_core PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
    )
    target_include_directories(epd_core PUBLIC $<BUILD_INTERFACE:${EPD_CORE_INCLUDE_DIR}>)
    target_compile_features(epd_core PUBLIC c_std_11)
endif()
