# MSI Control Center for Linux

A control center for MSI laptops on Linux: performance modes, custom fan curves, battery charge limit,
RGB keyboard lighting, GPU mode and more, from one Qt app with a tray icon.

It is built and tested on the **MSI Katana 15 HX B14WFK** (MS-1587) and works on other MSI laptops that the
[msi-ec](https://github.com/BeardOverflow/msi-ec) driver supports.

> Based on [MControlCenter](https://github.com/dmitry-s93/MControlCenter) by Dmitry Serov and contributors,
> licensed under the GPL-3.0. This project keeps that license.

## Features

**Performance and cooling**
- Modes: High Performance, Balanced, Silent, Super Battery
- Automatic mode switching on charger/battery, or following the system power profile (GNOME/KDE power menu)
- Custom fan curves for CPU and GPU, kept across mode changes, sleep and restarts
- Cooler Boost
- CPU/GPU temperatures and real fan speeds (RPM)

**Battery**
- Charge limit (60 %, 80 %, 100 % or custom), restored at start
- Battery health, capacity, charge cycles and current power draw

**Graphics**
- GPU tab: **Hybrid** (NVIDIA sleeps until needed) or **Integrated** (NVIDIA fully off for the longest battery life)
- Shows whether the dGPU is active, sleeping, or the laptop runs in discrete-GPU-only (MUX) mode

**Keyboard**
- 4-zone RGB lighting for the MSI Mystic Light MS-1565 controller (USB `1462:1601`, e.g. Katana 15/17 HX):
  static, breathing, color cycle, rainbow wave, per-zone colors, brightness, speed, save to keyboard memory
- Working touchpad on/off key on Katana HX laptops
- Fn ⇄ Win key swap, webcam on/off

**Desktop**
- Follows the GTK theme (colors, fonts, icons) with a GNOME-style header bar and sidebar;
  macOS-style window buttons with MacTahoe/WhiteSur themes
- Tray icon, start at login, single window
- Complete English and Hungarian translations (plus the translations of the original project)

## Requirements

- The [msi-ec](https://github.com/BeardOverflow/msi-ec) kernel driver for your laptop
  (check: `cat /sys/devices/platform/msi-ec/shift_mode`)
- `ec_sys` with write support for fan curves (configured automatically by the package)
- Qt 6.4+, polkit, D-Bus; UPower / power-profiles-daemon for automatic mode switching

## Installation

### Arch Linux / CachyOS

```
sudo pacman -S --needed base-devel git cmake ninja qt6-base qt6-tools
git clone https://github.com/RosenthalAlex/msi-control-center-linux.git
cd msi-control-center-linux/packaging/arch
makepkg -si
```

Update later with `git pull` in the cloned folder, then `makepkg -si` again in `packaging/arch`.

### Other distributions (CMake)

```
cmake -S . -B build -G Ninja -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
sudo systemctl enable mcontrolcenter-gpu.service
sudo systemd-hwdb update && sudo udevadm trigger --subsystem-match=input --action=change
```

Translations are built when Qt LinguistTools (`qt6-tools`) is installed. Reboot once after installing so the
D-Bus helper, its polkit rule and `ec_sys` are picked up.

## How it works

The app runs as your user. Hardware changes go through a small root helper (`mcontrolcenter-helper`)
that D-Bus starts on demand. Every write is checked with polkit (action `io.github.mcontrolcenter.control`):
the active local session is allowed without a password, other sessions must authenticate as an administrator.

### Look and feel

On GNOME and other GTK desktops the app uses Qt's GTK3 platform theme, so it picks up the GTK theme's
colors, fonts and icon theme. It draws its own header bar: the window buttons follow
`org.gnome.desktop.wm.preferences button-layout`, and with macOS-like themes (MacTahoe, WhiteSur) they are
drawn as traffic lights. Run with `MCC_NATIVE_DECORATIONS=1` for the system window frame and tabs instead.

### GPU mode

- **Hybrid**: the NVIDIA GPU sleeps until an app needs it.
- **Integrated**: the dGPU is unbound and removed from the PCI bus, so its PCIe port can cut its power.
  If the desktop is using the GPU, this is applied at the next boot by `mcontrolcenter-gpu.service`,
  before the display manager starts. Switching back to Hybrid is immediate.

- **Discrete**: the firmware MUX connects the display to the NVIDIA GPU (best for gaming on the charger).
  The app requests it the same way MSI Center does: a 2-bit code in the UEFI variable `MsiDCVarData` plus a
  firmware call (SMI 0x11), applied at the next boot. Switching back to Hybrid works the same way.
  Safeguards: only on MSI laptops, only with a known variable layout (seen on the Katana 15 HX B14WFK and the
  Vector 16 HX), only on the charger, not under kernel lockdown (Secure Boot), and every previous value is saved
  in `/var/lib/mcontrolcenter/mux-backup/`. Method from [msi-gpu-mode](https://github.com/ege-adam/msi-gpu-mode).
  On other laptops use the BIOS or MSI Center.

For the NVIDIA GPU to power down on its own in Hybrid mode, add
`options nvidia NVreg_DynamicPowerManagement=0x02` to `/etc/modprobe.d/nvidia-runtimepm.conf`
and rebuild the initramfs. Some BIOSes do not support this ("Runtime D3: Not supported" in
`/proc/driver/nvidia/gpus/*/power`); use Integrated mode there.

### RGB keyboard

The Mystic Light MS-1565 controller is a USB HID device, not part of the EC. The packet format comes from
[OpenRGB](https://openrgb.org/)'s `MSIKeyboard1565Controller`. For other RGB keyboards, try OpenRGB.

### Touchpad key on Katana HX

The key sends Ctrl + Super + F24, which desktops ignore. The package installs
`/usr/lib/udev/hwdb.d/61-mcontrolcenter-msi-keyboard.hwdb`, which turns it into the standard touchpad toggle key.

### Raw EC features on msi-ec systems

Keyboard backlight mode and USB Power Share are written straight into EC memory at fixed addresses. When
msi-ec is loaded they are hidden, because on newer firmware the same addresses control other things
(on the Katana 15 HX, `0x2C` is the mic-mute LED). If you know your model uses these addresses, add
`allowRawEcFeatures=true` under `[Settings]` in `~/.config/MControlCenter.conf`.

### NVIDIA power limits

Performance modes change the NVIDIA power limit only with the proprietary driver and `nvidia-powerd`:
`sudo systemctl enable --now nvidia-powerd`.

## Tested devices

| Laptop | Board | BIOS / EC | Notes |
|---|---|---|---|
| MSI Katana 15 HX B14WFK | MS-1587 | E1587IMS.108 / 1587EMS1.106 | Development machine, RTX 5060 Laptop |

Other models: see the [msi-ec device list](https://github.com/BeardOverflow/msi-ec/discussions/277).

## Translations

1. Copy `src/i18n/MControlCenter_en.ts` to `src/i18n/MControlCenter_xx.ts` (xx = language code).
2. Change `language="en_US"` to your language code.
3. Translate the strings in a text editor, Qt Linguist or Lokalize.
4. Add `GenericName[xx]=...` to `resources/mcontrolcenter.desktop`.
5. Open a pull request.

## License

GPL-3.0-or-later, see [LICENSE](LICENSE). Copyright of the original code: Dmitry Serov, Jérôme Lécuyer and the
MControlCenter contributors.
