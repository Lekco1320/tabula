set(EPD_ASSET_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EPD_ASSET_PRIV_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/priv_include)
set(EPD_ASSET_SRCS
    bitmap_asset.c
    bitmap_dither_matrix.c
    bitmap_generator.c
    font_asset.c
    font_face.c)

include(FetchContent)
FetchContent_Declare(freetype
    GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
    GIT_TAG VER-2-13-3
)
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(freetype)

add_library(epd_asset STATIC ${EPD_ASSET_SRCS})
set_target_properties(epd_asset PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF)
target_include_directories(epd_asset
    PUBLIC
        $<BUILD_INTERFACE:${EPD_ASSET_INCLUDE_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${EPD_ASSET_PRIV_INCLUDE_DIR}>)
target_link_libraries(epd_asset PUBLIC
    epd_core
    epd_gfx
    freetype)
target_compile_features(epd_asset PUBLIC c_std_11)
