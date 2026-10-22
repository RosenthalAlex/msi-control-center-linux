cmake_minimum_required(VERSION 3.25)

project(MControlCenterHelper LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core DBus)

add_executable(mcontrolcenter-helper
  helper.h helper.cpp
  readwrite.h readwrite.cpp
  msi-ec.h msi-ec.cpp
  keyboard-rgb.h keyboard-rgb.cpp
  polkit.h polkit.cpp
)
target_compile_options(mcontrolcenter-helper PRIVATE -Wall -Wextra)
target_link_libraries(mcontrolcenter-helper PRIVATE
  Qt6::Core
  Qt6::DBus
)

set(MCC_HELPER_DIR ${CMAKE_INSTALL_FULL_LIBEXECDIR})
configure_file(mcontrolcenter.helper.service.in
               ${CMAKE_CURRENT_BINARY_DIR}/mcontrolcenter.helper.service @ONLY)

install(TARGETS mcontrolcenter-helper RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
install(FILES mcontrolcenter-helper.conf
        DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system.d)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mcontrolcenter.helper.service
        DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system-services)
install(FILES io.github.mcontrolcenter.policy
        DESTINATION ${CMAKE_INSTALL_DATADIR}/polkit-1/actions)
