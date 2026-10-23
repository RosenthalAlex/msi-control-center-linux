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

#ifndef GPU_WIDGET_H
#define GPU_WIDGET_H

#include <QWidget>

class QDBusInterface;
class QLabel;
class QPushButton;
class QRadioButton;

/// "GPU" page: hybrid mode or integrated mode (dGPU fully off).
class GpuWidget : public QWidget {
    Q_OBJECT
public:
    explicit GpuWidget(QWidget *parent = nullptr);
    ~GpuWidget() override;

    /// False if there is no dGPU (and none was turned off by us) or no helper.
    [[nodiscard]] bool isRelevant() const;
    /// Re-reads the state from the helper.
    void refresh();

private:
    QDBusInterface *iface = nullptr;
    QLabel *stateLabel = nullptr;
    QLabel *infoLabel = nullptr;
    QRadioButton *hybridRadio = nullptr;
    QRadioButton *integratedRadio = nullptr;
    QPushButton *retryButton = nullptr;
    bool updating = false;

    QString call(const QString &method) const;
    void setMode(const QString &mode);
    void showPending();
};

#endif // GPU_WIDGET_H
