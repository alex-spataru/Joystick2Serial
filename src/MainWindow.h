/*
 * Copyright (c) 2022 Alex Spataru <https://github.com/alex-spataru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <QTimer>
#include <QCheckBox>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QProgressBar>

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void sendData();
    void refreshSerial();
    void refreshJoysticks();
    void onConnectButtonChanged();
    void onJoystickIndexChanged(int index);

    void connectSerial();
    void disconnectSerial();
    void onDeviceIndexChanged(int index);
    void onBaudRateIndexChanged(int index);
    void onSerialDataSent(const QByteArray &data);
    void onSerialDataReceived(const QByteArray &data);

    void onAxisChanged(const int js, const int axis, const qreal value);
    void onButtonChanged(const int js, const int button, const bool pressed);

private:
    Ui::MainWindow *m_ui;
    QString m_buffer;

    QVBoxLayout *m_axisLayout;
    QGridLayout *m_buttonsLayout;

    QList<QProgressBar *> m_axes;
    QList<QCheckBox *> m_buttons;

    double m_spd1;
    double m_spd2;
    double m_stp1;
    double m_stp2;

    QTimer m_sendTimer;
};
