set(EPD_GFX_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPD_GFX_PRIV_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/priv_include)
set(EPD_GFX_SRCS
    canvas.c
    codec.c
    glyph.c
    font.c
    font_asset.c
    sfnt_font.c)

function(epd_gfx_fetch_deps)
    include(FetchContent)
    FetchContent_Declare(stb_truetype_lib
        URL https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h
        DOWNLOAD_NO_EXTRACT TRUE
    )
    FetchContent_Declare(xxhash
        GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
        GIT_TAG v0.8.3
        SOURCE_SUBDIR cmake_unofficial
    )
    set(XXHASH_BUILD_XXHSUM OFF CACHE BOOL "" FORCE)
    set(XXHASH_BUILD_ENABLE_INLINE_API OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(stb_truetype_lib xxhash)

    set(EPD_GFX_STB_TRUETYPE_INCLUDE_DIR ${stb_truetype_lib_SOURCE_DIR} PARENT_SCOPE)
endfunction()

if(COMMAND idf_component_register)
    idf_component_register(
        REQUIRES
            epd_core
        INCLUDE_DIRS
            ${EPD_GFX_INCLUDE_DIR}
        PRIV_INCLUDE_DIRS
            ${EPD_GFX_PRIV_INCLUDE_DIR}
        SRCS
            ${EPD_GFX_SRCS})
    if(NOT CMAKE_SCRIPT_MODE_FILE)
        epd_gfx_fetch_deps()

        target_compile_options(xxhash PRIVATE -O2)
        target_include_directories(${COMPONENT_LIB} PRIVATE ${EPD_GFX_STB_TRUETYPE_INCLUDE_DIR})
        target_link_libraries(${COMPONENT_LIB} PRIVATE xxHash::xxhash)
    endif()
else()
    epd_gfx_fetch_deps()

    add_library(epd_gfx STATIC ${EPD_GFX_SRCS})
    set_target_properties(epd_gfx PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF)
    target_include_directories(epd_gfx
        PUBLIC
            $<BUILD_INTERFACE:${EPD_GFX_INCLUDE_DIR}>
        PRIVATE
            $<BUILD_INTERFACE:${EPD_GFX_PRIV_INCLUDE_DIR}>
            $<BUILD_INTERFACE:${EPD_GFX_STB_TRUETYPE_INCLUDE_DIR}>)
    target_link_libraries(epd_gfx PUBLIC
        epd_core
        xxHash::xxhash)
    target_compile_features(epd_gfx PUBLIC c_std_99)
endif()
