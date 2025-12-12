cmake_minimum_required(VERSION 3.21)

project(tabula-host VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Svg)

set(PROJECT_SOURCES
    main.cpp
    common/resources.qrc
    controls/CanvasPreviewer.hpp
    controls/CanvasPreviewer.cpp
    controls/MainWindow.hpp
    controls/MainWindow.cpp)

set(SHARED_LIBS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../shared)
add_subdirectory(${SHARED_LIBS_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/_shared)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(tabula-host
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES})
    message(STATUS "Qt version: ${Qt6_VERSION}")
else()
    add_executable(tabula-host
        ${PROJECT_SOURCES})
    message(STATUS "Qt version: ${Qt5_VERSION}")
endif()

include(FetchContent)
FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.2.1)
FetchContent_Declare(Qlementine
    GIT_REPOSITORY "https://github.com/oclero/qlementine.git"
    GIT_TAG v1.4.1)
FetchContent_MakeAvailable(fmt)
FetchContent_MakeAvailable(Qlementine)

target_include_directories(tabula-host PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/controls)

target_link_libraries(tabula-host PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    fmt::fmt
    qlementine
    epd_gfx
    epd_core)

include(GNUInstallDirs)
install(TARGETS tabula-host
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(tabula-host)
endif()
