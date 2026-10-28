/* Copyright (C) 2026  MControlCenter contributors
 *
 * This file is part of MControlCenter.
 *
 * MControlCenter is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * MControlCenter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MControlCenter. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GPU_SWITCH_H
#define GPU_SWITCH_H

#include "service.h"

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtDBus/QDBusAbstractAdaptor>

/**
 * Turns the discrete GPU fully off ("integrated" mode) or back on ("hybrid").
 *
 * Integrated: the dGPU's PCI functions (video + HDMI audio) are unbound from
 * their drivers and removed from the PCI bus, so nothing can wake them.
 * Hybrid: a PCI rescan brings them back and the drivers bind again.
 *
 * This only works with the MUX in hybrid position (the iGPU drives the panel).
 * Removing a GPU that is in use would crash its users, so it is only done
 * when no process has the device open; otherwise the choice is stored and
 * applied at the next boot by mcontrolcenter-gpu.service, before the desktop
 * starts.
 */
class GpuSwitch : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", INTERFACE_NAME_GPU)
public:
    explicit GpuSwitch(QObject *parent) : QDBusAbstractAdaptor(parent) {}

    /// Applies the saved mode; used by the boot service (no D-Bus).
    static int applySavedModeAtBoot();

public slots:
    /**
     * Current state: "hybrid", "integrated" (dGPU removed), "discrete"
     * (MUX set to dGPU only, switching not possible) or "none" (no dGPU).
     */
    [[nodiscard]] QString getMode() const;
    /// Mode stored for the next boot: "hybrid" or "integrated".
    [[nodiscard]] QString getSavedMode() const;
    /**
     * Stores and tries to apply the mode. Returns "applied", "pending"
     * (stored; the GPU is in use, see getBlockingProcesses) or "error: ...".
     */
    QString setMode(const QString &mode) const;
    /// Names of processes that keep the dGPU open.
    [[nodiscard]] QStringList getBlockingProcesses() const;

    // ---- Firmware MUX (Hybrid <-> Discrete, applies after a reboot) ----
    /// Mode the firmware booted with: "hybrid", "discrete", "integrated", "unknown"
    [[nodiscard]] QString getMuxApplied() const;
    /// Mode requested for the next boot
    [[nodiscard]] QString getMuxRequested() const;
    /// Empty when switching is possible, otherwise the reason it is not
    [[nodiscard]] QString getMuxUnavailableReason() const;
    /// "hybrid" or "discrete". Returns "reboot", "ok" (nothing to change) or "error: ..."
    QString setMuxMode(const QString &mode) const;
};

#endif // GPU_SWITCH_H
