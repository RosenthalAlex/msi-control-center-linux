#!/bin/bash -e
# Installs a binary bundle made by create_installer.sh. Run with sudo.

BIN_PATH='/usr/bin/'
LIB_EXEC_PATH='/usr/libexec/'
SCALABLE_ICONS_PATH='/usr/share/icons/hicolor/scalable/apps/'
SHORTCUTS_PATH='/usr/share/applications/'
DBUS_SYSTEM_PATH='/usr/share/dbus-1/system.d/'
DBUS_SERVICES_PATH='/usr/share/dbus-1/system-services/'
POLKIT_ACTIONS_PATH='/usr/share/polkit-1/actions/'
MODPROBE_PATH='/usr/lib/modprobe.d/'

APP_DIR='./app/'

echo "Installation start"

install -vDm755 ${APP_DIR}mcontrolcenter ${BIN_PATH}mcontrolcenter

if [ -n "$SUDO_USER" ]; then
    rm -fv "/home/$SUDO_USER/.local/share/applications/mcontrolcenter.desktop"
fi
install -vDm644 ${APP_DIR}mcontrolcenter.desktop ${SHORTCUTS_PATH}mcontrolcenter.desktop
install -vDm644 ${APP_DIR}mcontrolcenter.svg ${SCALABLE_ICONS_PATH}mcontrolcenter.svg
install -vDm755 ${APP_DIR}mcontrolcenter-helper ${LIB_EXEC_PATH}mcontrolcenter-helper
install -vDm644 ${APP_DIR}mcontrolcenter-helper.conf ${DBUS_SYSTEM_PATH}mcontrolcenter-helper.conf
install -vDm644 ${APP_DIR}mcontrolcenter.helper.service ${DBUS_SERVICES_PATH}mcontrolcenter.helper.service
install -vDm644 ${APP_DIR}io.github.mcontrolcenter.policy ${POLKIT_ACTIONS_PATH}io.github.mcontrolcenter.policy
install -vDm644 ${APP_DIR}mcontrolcenter-ec_sys.conf ${MODPROBE_PATH}mcontrolcenter.conf

echo "Installation was successful"
