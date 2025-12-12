set(EPD_CORE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPD_CORE_SRCS common.c)

if(COMMAND idf_component_register)
    idf_component_register(
        INCLUDE_DIRS
            ${EPD_CORE_INCLUDE_DIR}
        SRCS
            ${EPD_CORE_SRCS})
else()
    add_library(epd_core STATIC ${EPD_CORE_SRCS})
    target_include_directories(epd_core PUBLIC $<BUILD_INTERFACE:${EPD_CORE_INCLUDE_DIR}>)
    target_compile_features(epd_core PUBLIC c_std_99)
endif()