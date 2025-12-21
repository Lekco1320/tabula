cmake_minimum_required(VERSION 3.16)

get_filename_component(SHARED_DIR "${CMAKE_CURRENT_LIST_DIR}/../shared" ABSOLUTE)
set(EXTRA_COMPONENT_DIRS 
    "${SHARED_DIR}/epd_core"
    "${SHARED_DIR}/epd_gfx"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)
set(PROJECT_VER "0.0.0")
project(epaper)
