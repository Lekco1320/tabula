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
    GIT_TAG v1.4.2)
FetchContent_MakeAvailable(Qlementine)

add_subdirectory(${SHARED_LIBS_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/_shared)

qt_add_executable(tabula-host
    WIN32
    MANUAL_FINALIZATION)

target_sources(tabula-host PRIVATE
    main.cpp
    resources.qrc
    controls/Utils.cpp
    controls/Utils.hpp)

target_sources(tabula-host PRIVATE
    controls/bars/CursorBar.cpp
    controls/bars/CursorBar.hpp
    controls/bars/IconButtonBar.cpp
    controls/bars/IconButtonBar.hpp
    controls/bars/RotationBar.cpp
    controls/bars/RotationBar.hpp
    controls/bars/ToolBar.cpp
    controls/bars/ToolBar.hpp)

target_sources(tabula-host PRIVATE
    controls/panels/ControlPanel.cpp
    controls/panels/ControlPanel.hpp
    controls/panels/FillPanel.cpp
    controls/panels/FillPanel.hpp
    controls/panels/LinePanel.cpp
    controls/panels/LinePanel.hpp
    controls/panels/PixelPanel.cpp
    controls/panels/PixelPanel.hpp
    controls/panels/RectPanel.cpp
    controls/panels/RectPanel.hpp
    controls/panels/ToolPanel.cpp
    controls/panels/ToolPanel.hpp)

target_sources(tabula-host PRIVATE
    controls/widgets/AdaptiveStackedWidget.cpp
    controls/widgets/AdaptiveStackedWidget.hpp
    controls/widgets/CanvasPreviewer.cpp
    controls/widgets/CanvasPreviewer.hpp
    controls/widgets/ColorButton.cpp
    controls/widgets/ColorButton.hpp
    controls/widgets/FlowLayout.cpp
    controls/widgets/FlowLayout.hpp)

target_sources(tabula-host PRIVATE
    controls/windows/MainWindow.cpp
    controls/windows/MainWindow.hpp
    controls/windows/SetupDialog.cpp
    controls/windows/SetupDialog.hpp)

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
