set(EPD_CORE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(COMMAND idf_component_register)
    idf_component_register(
        INCLUDE_DIRS
            ${EPD_CORE_INCLUDE_DIR})
else()
    add_library(epd_core INTERFACE)
    target_include_directories(epd_core INTERFACE $<BUILD_INTERFACE:${EPD_CORE_INCLUDE_DIR}>)
    target_compile_features(epd_core INTERFACE c_std_99)
endif()