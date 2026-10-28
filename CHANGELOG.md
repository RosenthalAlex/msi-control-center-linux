# Changelog

All notable changes to this project will be documented in this file.

## Unreleased
### Changed
- New look that follows the GTK theme: Qt's GTK3 platform theme supplies colors, fonts, icons and
  file dialogs on GNOME-like desktops. A GNOME-style header bar (window buttons on the side set in
  `button-layout`, macOS "traffic lights" with MacTahoe/WhiteSur themes), a sidebar instead of tabs,
  rounded cards, pill buttons and modern sliders. Set `MCC_NATIVE_DECORATIONS=1` to get the
  system's window frame and the old tab layout back.

## [0.6.0] 2026-10-23
### Added
- RGB keyboard lighting for the MSI Mystic Light MS-1565 controller (USB 1462:1601, Katana 15/17 HX):
  off, static, breathing, color cycle and rainbow wave; per-zone colors (4 zones), brightness,
  speed, wave direction, color presets, restore on start/after sleep, save to keyboard memory
- Real fan speeds (RPM) from the `msi_wmi_platform` driver on newer models
- Battery health: health %, full/design capacity, charge cycles, battery model
- Discrete GPU power state (active/sleeping, discrete-only MUX mode) and battery power draw on the overview page
- polkit authorization for every write call to the root helper (`io.github.mcontrolcenter.control`,
  allowed without a password for the active local session)
- `cmake --install` support (GNUInstallDirs), Arch Linux / CachyOS `PKGBUILD`, AppStream metainfo,
  modprobe config that enables `ec_sys` write support
- Complete Hungarian translation
- Touchpad on/off key works on Katana 15/17 HX (udev hwdb maps its Ctrl+Super+F24 to the touchpad toggle key)
- "Start automatically at login" option; launching the app again raises the running window
- Charge limit is re-applied at start
- The custom fan curve is kept: it is re-applied after every mode change (manual, charger or system
  power profile), after sleep and at start, and a 10 s check puts it back if the EC drops it.
  Only clicking the checkbox changes the saved choice.
- GPU page: Hybrid or Integrated mode. Integrated unbinds and removes the dGPU from the PCI bus
  (like supergfxctl); if the desktop is using it, the mode is applied at the next boot by
  `mcontrolcenter-gpu.service` before the display manager starts. Back to Hybrid is immediate.
  Needs the MUX in hybrid position (BIOS); with "discrete GPU only" the page explains how to change it.

### Changed
- Raw EC writes for keyboard backlight mode and USB power share are disabled on msi-ec systems,
  because their fixed EC addresses can belong to other functions (on Katana 15 HX, 0x2C is the
  mic-mute LED). Set `Settings/allowRawEcFeatures=true` in `~/.config/MControlCenter.conf` to re-enable.
- Feature support is queried once instead of on every refresh (far fewer D-Bus calls)
- Translations are optional at build time (Qt LinguistTools no longer required)
- Tabs are addressed by widget instead of by index

### Fixed
- Crash when a sysfs file is empty or unreadable in the helper
- Crash on corrupt fan curve values in the settings file
- Out-of-range EC addresses (e.g. -1 for undetected features) could be passed to the helper
- `modprobe` is found on distributions without `/usr/sbin`; the `QProcess` used for it leaked

## [0.4.1] 2023-08-09
- Fixed UI lags when resizing the window (issue https://github.com/dmitry-s93/MControlCenter/issues/97)
- Updated Norwegian Bokmål (Thanks [msigurdsen](https://github.com/msigurdsen))
- Added support for German language (Thanks [EchterAlsFake](https://github.com/EchterAlsFake))
- Added support for Hungarian language (Thanks [BiRo96](https://github.com/BiRo96))

## [0.4.0] 2023-03-06
- Added fan control support
- Updated Norwegian Bokmål (Thanks [msigurdsen](https://github.com/msigurdsen))
- Updated Spanish translation (Thanks [rottenpants466](https://www.github.com/rottenpants466))
- Updated Italian translation (Thanks [rottenpants466](https://www.github.com/rottenpants466))
- Updated Portuguese translation (Thanks [rottenpants466](https://www.github.com/rottenpants466))

## [0.3.2] 2022-11-28
- Added fan mode display
- Added description on mode tab
- Added "Cooler Boost" item to tray menu
- Added support for [acpi_ec](https://github.com/musikid/acpi_ec) kernel module
- Added support for Italian language (Thanks [marraviglioso](https://github.com/marraviglioso))
- Added support for Portuguese (Brazilian) language (Thanks [rottenpants466](https://www.github.com/rottenpants466))
- Updated Spanish translation (Thanks [rottenpants466](https://www.github.com/rottenpants466))
- Updated Portuguese translation (Thanks [rottenpants466](https://www.github.com/rottenpants466))
- Fixed display of the window when clicking the tray icon if the application was minimized

## [0.3.1] 2022-11-04
- Added support for Norwegian Bokmål (Thanks [msigurdsen](https://github.com/msigurdsen))
- Added support for Turkish language (Thanks [emrgncr](https://github.com/emrgncr))
- Corrected display of the current battery level
- Fixed fan 1 speed display for some devices
- Fixed "FN <-> Super" for some devices (Issue https://github.com/dmitry-s93/MControlCenter/issues/53)

## [0.3.0] 2022-10-28
- Added support for Spanish language (Thanks [ssergio-ll](https://www.github.com/ssergio-ll))
- Ability to set the battery charge limit from 30% to 100% in steps of 1
- Prevent launching more than one instance
- Added application icon to system tray
- Added saving and restoring settings
- Some optimizations
- Fixed turning on/off webcam

## [0.2.0] 2022-10-16
- Added mode change (High Performance, Balanced, Silent, Super Battery)
- Added support for Portuguese language (Thanks [rottenpants466](https://www.github.com/rottenpants466))
- Run helper as D-BUS service
- Some optimizations
- Fixed typos

## [0.1.2] 2022-10-02

- GUI improvements
- Some optimizations
- Added link to bug tracker
- Added "Fully charged" battery status
- Added "Fully charged (Discharging)" battery status

## [0.1.1] 2022-09-21

- Added charging status display
- Added localization support
- Added support for Russian language
- Some optimizations

## [0.1.0 alpha] 2022-09-12

- First release
