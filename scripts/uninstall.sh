#!/bin/bash -e

SHORTCUT='mcontrolcenter.desktop'

echo "Start uninstall"

rm -fv /usr/bin/mcontrolcenter
rm -fv /usr/share/applications/$SHORTCUT
[ -n "$SUDO_USER" ] && rm -fv "/home/$SUDO_USER/.local/share/applications/$SHORTCUT"
rm -fv /usr/share/icons/hicolor/scalable/apps/mcontrolcenter.svg
rm -fv /usr/libexec/mcontrolcenter-helper
rm -fv /etc/dbus-1/system.d/mcontrolcenter-helper.conf
rm -fv /usr/share/dbus-1/system.d/mcontrolcenter-helper.conf
rm -fv /usr/share/dbus-1/system-services/mcontrolcenter.helper.service
rm -fv /usr/share/polkit-1/actions/io.github.mcontrolcenter.policy
rm -fv /usr/lib/modprobe.d/mcontrolcenter.conf
rm -fv /usr/lib/udev/hwdb.d/61-mcontrolcenter-msi-keyboard.hwdb
systemd-hwdb update || true

echo "Uninstall was successful"
