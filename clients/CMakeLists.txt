cmake_minimum_required(VERSION 3.21)

project(tabula-host VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SHARED_LIBS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../shared)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)
message(STATUS "Qt version: ${Qt6_VERSION}")

include(FetchContent)
set(QLEMENTINE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(Qlementine
    GIT_REPOSITORY "https://github.com/oclero/qlementine.git"
    GIT_TAG v1.4.1)
FetchContent_MakeAvailable(Qlementine)

add_subdirectory(${SHARED_LIBS_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/_shared)

file(GLOB TABULA_CONTROL_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/controls/**/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/controls/**/*.hpp")

set(TABULA_PROJECT_SRCS
    main.cpp
    resources.qrc
    ${TABULA_CONTROL_SRCS})

qt_add_executable(tabula-host
    WIN32
    MANUAL_FINALIZATION
    ${TABULA_PROJECT_SRCS})

set_target_properties(tabula-host PROPERTIES
    AUTOMOC ON
    AUTOUIC OFF
    AUTORCC ON)

target_include_directories(tabula-host PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(tabula-host PRIVATE
    Qt6::Widgets
    Qt6::Svg
    qlementine
    epd_gfx
    epd_core)

qt_generate_deploy_app_script(
    TARGET tabula-host
    OUTPUT_SCRIPT qt_deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_TOOL_OPTIONS
        --no-translations
        --no-system-d3d-compiler
        --no-opengl-sw)

include(GNUInstallDirs)
install(TARGETS tabula-host
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT Runtime)
install(SCRIPT ${qt_deploy_script}
    COMPONENT Runtime)

qt_finalize_executable(tabula-host)

include(CPack)
