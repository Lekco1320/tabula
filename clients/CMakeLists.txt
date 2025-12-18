cmake_minimum_required(VERSION 3.21)

project(tabula-host VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)

file(GLOB TABULA_CONTROL_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/controls/**/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/controls/**/*.hpp")

set(TABULA_PROJECT_SRCS
    main.cpp
    resources.qrc
    ${TABULA_CONTROL_SRCS})

set(SHARED_LIBS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../shared)
add_subdirectory(${SHARED_LIBS_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/_shared)

qt_add_executable(tabula-host
    MANUAL_FINALIZATION
    ${TABULA_PROJECT_SRCS})
message(STATUS "Qt version: ${Qt6_VERSION}")

include(FetchContent)
FetchContent_Declare(Qlementine
    GIT_REPOSITORY "https://github.com/oclero/qlementine.git"
    GIT_TAG v1.4.1)
FetchContent_MakeAvailable(Qlementine)

target_include_directories(tabula-host PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(tabula-host PRIVATE
    Qt6::Widgets
    Qt6::Svg
    qlementine
    epd_gfx
    epd_core)

include(GNUInstallDirs)
install(TARGETS tabula-host
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

qt_finalize_executable(tabula-host)
