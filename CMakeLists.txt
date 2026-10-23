cmake_minimum_required(VERSION 3.25)

project(MControlCenter VERSION 0.6.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# Installed binaries use system Qt; no RPATH (the helper runs as root)
set(CMAKE_SKIP_INSTALL_RPATH ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(MCC_TRANSLATIONS "Build and embed translations (needs Qt6 LinguistTools)" ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets DBus)

qt_standard_project_setup()

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.ui
    src/mainwindow.cpp src/mainwindow.h
    src/operate.cpp src/operate.h
    src/powermonitor.cpp src/powermonitor.h
    src/helper.cpp src/helper.h
    src/msi-ec_helper.cpp src/msi-ec_helper.h
    src/keyboard_rgb.cpp src/keyboard_rgb.h
    src/keyboard_rgb_widget.cpp src/keyboard_rgb_widget.h
    src/sysinfo.cpp src/sysinfo.h
    src/gpu_widget.cpp src/gpu_widget.h
    src/settings.cpp src/settings.h
    src/resources.qrc
)

qt_add_executable(mcontrolcenter ${PROJECT_SOURCES})

target_compile_options(mcontrolcenter PRIVATE -Wall -Wextra)

target_link_libraries(mcontrolcenter PRIVATE
    Qt6::Widgets
    Qt6::DBus
)

target_compile_definitions(mcontrolcenter PRIVATE
    MControlCenter_VERSION="${PROJECT_VERSION}"
)

# Translations are optional so the app can be built with only qt6-base installed.
if(MCC_TRANSLATIONS)
    find_package(Qt6 QUIET COMPONENTS LinguistTools)
endif()

if(MCC_TRANSLATIONS AND Qt6LinguistTools_FOUND)
    file(GLOB TS_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/i18n/MControlCenter_*.ts)
    qt_add_translations(mcontrolcenter
        TS_FILES ${TS_FILES}
        RESOURCE_PREFIX "/translations"
        LUPDATE_OPTIONS -no-obsolete -locations none
    )
    target_compile_definitions(mcontrolcenter PRIVATE MCC_HAS_TRANSLATIONS)
else()
    message(STATUS "Qt6 LinguistTools not found (or MCC_TRANSLATIONS=OFF): building without translations")
endif()

add_subdirectory(src/helper ${CMAKE_BINARY_DIR}/helper)

# ---- Installation ----
install(TARGETS mcontrolcenter RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES resources/mcontrolcenter.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES resources/mcontrolcenter.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES resources/mcontrolcenter.appdata.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
        RENAME mcontrolcenter.metainfo.xml)
# ec_sys is needed (with write support) for custom fan curves
install(FILES resources/mcontrolcenter-ec_sys.conf
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/modprobe.d
        RENAME mcontrolcenter.conf)
# Makes the touchpad key work on MSI Katana HX laptops
install(FILES resources/udev/61-mcontrolcenter-msi-keyboard.hwdb
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/udev/hwdb.d)
