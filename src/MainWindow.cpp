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

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Serial.h"
#include "Utilities.h"
#include "QJoysticks.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);
    m_axisLayout = new QVBoxLayout(m_ui->axesContainer);
    m_buttonsLayout = new QGridLayout(m_ui->buttonsContainer);

    m_axisLayout->setMargin(0);
    m_axisLayout->setSpacing(6);
    m_buttonsLayout->setMargin(0);

    m_ui->axesContainer->setLayout(m_axisLayout);
    m_ui->buttonsContainer->setLayout(m_buttonsLayout);

    connect(&Serial::instance(), &Serial::dataSent, this, &MainWindow::onSerialDataSent);
    connect(&Serial::instance(), &Serial::dataReceived, this, &MainWindow::onSerialDataReceived);
    connect(&Serial::instance(), &Serial::availablePortsChanged, this, &MainWindow::refreshSerial);

    connect(QJoysticks::getInstance(), &QJoysticks::axisChanged, this, &MainWindow::onAxisChanged);
    connect(QJoysticks::getInstance(), &QJoysticks::buttonChanged, this, &MainWindow::onButtonChanged);
    connect(QJoysticks::getInstance(), &QJoysticks::countChanged, this, &MainWindow::refreshJoysticks);

    connect(m_ui->connectButton, &QCheckBox::clicked, this, &MainWindow::onConnectButtonChanged);
    connect(m_ui->baudRates, SIGNAL(currentIndexChanged(int)), this, SLOT(onBaudRateIndexChanged(int)));
    connect(m_ui->serialDevices, SIGNAL(currentIndexChanged(int)), this, SLOT(onDeviceIndexChanged(int)));
    connect(m_ui->joystickList, SIGNAL(currentIndexChanged(int)), this, SLOT(onJoystickIndexChanged(int)));

    m_ui->baudRates->clear();
    m_ui->baudRates->addItems(Serial::instance().baudRateList());
    m_ui->baudRates->setCurrentIndex(Serial::instance().baudRateList().indexOf("115200"));

    connect(&m_sendTimer, &QTimer::timeout, this, &MainWindow::sendData);
    m_sendTimer.start(100);
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

void MainWindow::sendData() {
    QString buffer = "$";
    const int js = m_ui->joystickList->currentIndex();

    if (QJoysticks::getInstance()->joystickExists(js)) {
        const int axes = QJoysticks::getInstance()->getNumAxes(js);
        const int buttons = QJoysticks::getInstance()->getNumButtons(js);

        if (axes <= 0 && buttons <= 0)
            return;

        for (int i = 0; i < axes; ++i) {
            auto val = QJoysticks::getInstance()->getAxis(js, i) * 100;
            buffer.append(QString::number((int) val));
            buffer.append(",");
        }

        for (int i = 0; i < buttons; ++i) {
            auto val = QJoysticks::getInstance()->getButton(js, i);
            buffer.append(val ? "H" : "L");
            buffer.append(",");
        }

        buffer.chop(1);
        buffer.append("\n");

        if (Serial::instance().isOpen())
            Serial::instance().write(buffer.toUtf8());
    }
}

void MainWindow::refreshSerial()
{
    auto index = m_ui->serialDevices->currentIndex();

    m_ui->serialDevices->clear();
    m_ui->serialDevices->addItems(Serial::instance().portList());

    if (index < m_ui->serialDevices->count() && index > 0)
        m_ui->serialDevices->setCurrentIndex(index);
    else
        m_ui->serialDevices->setCurrentIndex(0);
}

void MainWindow::refreshJoysticks() {
    m_ui->joystickList->clear();
    m_ui->joystickList->addItems(QJoysticks::getInstance()->deviceNames());
    m_ui->joystickList->setCurrentIndex(0);
}

void MainWindow::onConnectButtonChanged() {
    if (Serial::instance().isOpen())
        disconnectSerial();
    else
        connectSerial();
}

void MainWindow::onJoystickIndexChanged(int index) {
    if (!QJoysticks::getInstance()->joystickExists(index))
        return;

    for (int i = 0; i < m_axes.count(); ++i) {
        m_axisLayout->removeWidget(m_axes.at(i));
        m_axes.at(i)->deleteLater();
    }

    for (int i = 0; i < m_buttons.count(); ++i) {
        m_buttonsLayout->removeWidget(m_buttons.at(i));
        m_buttons.at(i)->deleteLater();
    }

    m_axes.clear();
    m_buttons.clear();

    for (int i = 0; i < QJoysticks::getInstance()->getNumAxes(index); ++i) {
        auto progress = new QProgressBar(this);
        progress->setMinimumHeight(12);
        progress->setMinimum(-100);
        progress->setMaximum(100);

        m_axisLayout->addWidget(progress);
        m_axes.append(progress);
    }

    int row = 0;
    int column = 0;
    for (int i = 0; i < QJoysticks::getInstance()->getNumButtons(index); ++i) {
        auto button = new QCheckBox(this);
        button->setText(tr("Butón %1").arg(i + 1));
        button->setAttribute(Qt::WA_TransparentForMouseEvents);
        button->setFocusPolicy(Qt::NoFocus);

        m_buttonsLayout->addWidget(button, row, column);
        m_buttons.append(button);

        ++column;
        if (column > 2) {
            column = 0;
            ++row;
        }
    }
}

void MainWindow::connectSerial() {
    if (!Serial::instance().open(QFile::ReadWrite)) {
        Utilities::showMessageBox("Error al abrir el puerto serial",
                                  "Favor de checar si el puerto no esta siendo utilizado por otro programa");
    }

    else {
        m_ui->connectButton->setChecked(true);
        m_ui->connectButton->setText("Desconectar");
    }
}

void MainWindow::disconnectSerial() {
    Serial::instance().close();
    m_ui->connectButton->setChecked(false);
    m_ui->connectButton->setText("Conectar");
}

void MainWindow::onDeviceIndexChanged(int index) {
    Serial::instance().setPortIndex(index);
    m_ui->connectButton->setEnabled(Serial::instance().configurationOk());
}

void MainWindow::onBaudRateIndexChanged(int index) {
    auto baud = Serial::instance().baudRateList().at(index);
    Serial::instance().setBaudRate(baud.toInt());
}

void MainWindow::onSerialDataSent(const QByteArray& data) {
    m_ui->console->append("<font color='#f88'><strong>TX:</strong> " + QString::fromUtf8(data) + "</font>");
}

void MainWindow::onSerialDataReceived(const QByteArray& data) {
    auto text = QString::fromUtf8(data).replace("\r\n", "\n");
    m_buffer.append(text);

    while (m_buffer.contains("\n")) {
        auto currentLine = m_buffer.split("\n").first().replace("\n", "");
        if (currentLine.isEmpty()) {
            m_buffer.remove(0, 1);
            continue;
        }

        m_ui->currentLine->setText(currentLine);
        m_ui->console->append("<font color='#88f'><strong>RX:</strong> " + currentLine + "</font>");
        m_buffer.remove(0, currentLine.length() + 1);
    }
}

void MainWindow::onAxisChanged(const int js, const int axis, const qreal value) {
    if (js == m_ui->joystickList->currentIndex()) {
        if (axis < m_axes.count())
            m_axes.at(axis)->setValue(value * 100);
    }
}

void MainWindow::onButtonChanged(const int js, const int button, const bool pressed) {
    if (js == m_ui->joystickList->currentIndex()) {
        if (button < m_buttons.count())
            m_buttons.at(button)->setChecked(pressed);
    }
}
