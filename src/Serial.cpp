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

#include "Serial.h"

//----------------------------------------------------------------------------------------
// Constructor/destructor & singleton access functions
//----------------------------------------------------------------------------------------

/**
 * Constructor function
 */
Serial::Serial()
    : m_port(Q_NULLPTR)
    , m_autoReconnect(false)
    , m_lastSerialDeviceIndex(0)
    , m_portIndex(0)
{
    // Read settings
    readSettings();

    // Init serial port configuration variables
    setBaudRate(9600);
    disconnectDevice();
    setDataBits(dataBitsList().indexOf("8"));
    setStopBits(stopBitsList().indexOf("1"));
    setParity(parityList().indexOf(tr("None")));
    setFlowControl(flowControlList().indexOf(tr("None")));

    // clang-format off

    // Build serial devices list and refresh it every second
    connect(&m_refreshTimer, &QTimer::timeout, this, &Serial::refreshSerialDevices);
    m_refreshTimer.start(1000);

    // Update connect button status when user selects a serial device
    connect(this, &Serial::portIndexChanged,
            this, &Serial::configurationChanged);

    // clang-format on
}

/**
 * Destructor function, closes the serial port before exiting the application and saves
 * the user's baud rate list settings.
 */
Serial::~Serial()
{
    writeSettings();

    if (port())
        disconnectDevice();
}

/**
 * Returns the only instance of the class
 */
Serial &Serial::instance()
{
    static Serial singleton;
    return singleton;
}

//----------------------------------------------------------------------------------------
// HAL-driver implementation
//----------------------------------------------------------------------------------------

/**
 * Closes the current serial port connection
 */
void Serial::close()
{
    if (isOpen())
        port()->close();
}

/**
 * Returns @c true if a serial port connection is currently open
 */
bool Serial::isOpen() const
{
    if (port())
        return port()->isOpen();

    return false;
}

/**
 * Returns @c true if the current serial device is readable
 */
bool Serial::isReadable() const
{
    if (isOpen())
        return port()->isReadable();

    return false;
}

/**
 * Returns @c true if the current serial device is writable
 */
bool Serial::isWritable() const
{
    if (isOpen())
        return port()->isWritable();

    return false;
}

/**
 * Returns @c true if the user selects the appropiate controls & options to be able
 * to connect to a serial device
 */
bool Serial::configurationOk() const
{
    return portIndex() > 0;
}

/**
 * Writes the given @a data to the serial device and returns the number of bytes written
 */
quint64 Serial::write(const QByteArray &data)
{
    if (isWritable())
    {
        auto bytes = port()->write(data);
        Q_EMIT dataSent(data.chopped(data.length() - bytes));
        return bytes;
    }

    return -1;
}

/**
 * Connects to the currently selected serial port device, returns @c true on success
 */
bool Serial::open(const QIODevice::OpenMode mode)
{
    // Ignore the first item of the list (Select Port)
    auto ports = validPorts();
    auto portId = portIndex() - 1;
    if (portId >= 0 && portId < validPorts().count())
    {
        // Update port index variable & disconnect from current serial port
        disconnectDevice();
        m_portIndex = portId + 1;
        m_lastSerialDeviceIndex = m_portIndex;
        Q_EMIT portIndexChanged();

        // Create new serial port handler
        m_port = new QSerialPort(ports.at(portId));

        // Configure serial port
        port()->setParity(parity());
        port()->setBaudRate(baudRate());
        port()->setDataBits(dataBits());
        port()->setStopBits(stopBits());
        port()->setFlowControl(flowControl());

        // Connect signals/slots
        connect(port(), SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this,
                SLOT(handleError(QSerialPort::SerialPortError)));

        // Open device
        if (port()->open(mode))
        {
            connect(port(), &QIODevice::readyRead, this, &Serial::onReadyRead);
            return true;
        }
    }

    // Disconnect serial port
    disconnectDevice();
    return false;
}

//----------------------------------------------------------------------------------------
// Driver specifics
//----------------------------------------------------------------------------------------

/**
 * Returns the name of the current serial port device
 */
QString Serial::portName() const
{
    if (port())
        return port()->portName();

    return tr("No Device");
}

/**
 * Returns the pointer to the current serial port handler
 */
QSerialPort *Serial::port() const
{
    return m_port;
}

/**
 * Returns @c true if auto-reconnect is enabled
 */
bool Serial::autoReconnect() const
{
    return m_autoReconnect;
}

/**
 * Returns the index of the current serial device selected by the program.
 */
quint8 Serial::portIndex() const
{
    return m_portIndex;
}

/**
 * Returns the correspoding index of the parity configuration in relation
 * to the @c QStringList returned by the @c parityList() function.
 */
quint8 Serial::parityIndex() const
{
    return m_parityIndex;
}

/**
 * Returns the correspoding index of the data bits configuration in relation
 * to the @c QStringList returned by the @c dataBitsList() function.
 */
quint8 Serial::dataBitsIndex() const
{
    return m_dataBitsIndex;
}

/**
 * Returns the correspoding index of the stop bits configuration in relation
 * to the @c QStringList returned by the @c stopBitsList() function.
 */
quint8 Serial::stopBitsIndex() const
{
    return m_stopBitsIndex;
}

/**
 * Returns the correspoding index of the flow control config. in relation
 * to the @c QStringList returned by the @c flowControlList() function.
 */
quint8 Serial::flowControlIndex() const
{
    return m_flowControlIndex;
}

/**
 * Returns a list with the available serial devices/ports to use.
 * This function can be used with a combo box to build nice UIs.
 *
 * @note The first item of the list will be invalid, since it's value will
 *       be "Select Serial Device". This is inteded to make the user interface
 *       a little more friendly.
 */
QStringList Serial::portList() const
{
    return m_portList;
}

/**
 * Returns a list with the available parity configurations.
 * This function can be used with a combo-box to build UIs.
 */
QStringList Serial::parityList() const
{
    QStringList list;
    list.append(tr("None"));
    list.append(tr("Even"));
    list.append(tr("Odd"));
    list.append(tr("Space"));
    list.append(tr("Mark"));
    return list;
}

/**
 * Returns a list with the available baud rate configurations.
 * This function can be used with a combo-box to build UIs.
 */
QStringList Serial::baudRateList() const
{
    return m_baudRateList;
}

/**
 * Returns a list with the available data bits configurations.
 * This function can be used with a combo-box to build UIs.
 */
QStringList Serial::dataBitsList() const
{
    return QStringList { "5", "6", "7", "8" };
}

/**
 * Returns a list with the available stop bits configurations.
 * This function can be used with a combo-box to build UIs.
 */
QStringList Serial::stopBitsList() const
{
    return QStringList { "1", "1.5", "2" };
}

/**
 * Returns a list with the available flow control configurations.
 * This function can be used with a combo-box to build UIs.
 */
QStringList Serial::flowControlList() const
{
    QStringList list;
    list.append(tr("None"));
    list.append("RTS/CTS");
    list.append("XON/XOFF");
    return list;
}

/**
 * Returns the current parity configuration used by the serial port
 * handler object.
 */
QSerialPort::Parity Serial::parity() const
{
    return m_parity;
}

/**
 * Returns the current baud rate configuration used by the serial port
 * handler object.
 */
qint32 Serial::baudRate() const
{
    return m_baudRate;
}

/**
 * Returns the current data bits configuration used by the serial port
 * handler object.
 */
QSerialPort::DataBits Serial::dataBits() const
{
    return m_dataBits;
}

/**
 * Returns the current stop bits configuration used by the serial port
 * handler object.
 */
QSerialPort::StopBits Serial::stopBits() const
{
    return m_stopBits;
}

/**
 * Returns the current flow control configuration used by the serial
 * port handler object.
 */
QSerialPort::FlowControl Serial::flowControl() const
{
    return m_flowControl;
}

/**
 * Disconnects from the current serial device and clears temp. data
 */
void Serial::disconnectDevice()
{
    // Check if serial port pointer is valid
    if (port() != Q_NULLPTR)
    {
        // Disconnect signals/slots
        port()->disconnect(this, SLOT(onReadyRead()));
        port()->disconnect(this, SLOT(handleError(QSerialPort::SerialPortError)));

        // Close & delete serial port handler
        port()->close();
        port()->deleteLater();
    }

    // Reset pointer
    m_port = Q_NULLPTR;
    Q_EMIT portChanged();
    Q_EMIT availablePortsChanged();
}

/**
 * Changes the baud @a rate of the serial port
 */
void Serial::setBaudRate(const qint32 rate)
{
    // Asserts
    Q_ASSERT(rate > 10);

    // Update baud rate
    m_baudRate = rate;

    // Update serial port config
    if (port())
        port()->setBaudRate(baudRate());

    // Update user interface
    Q_EMIT baudRateChanged();
}

/**
 * Changes the port index value, this value is later used by the @c openSerialPort()
 * function.
 */
void Serial::setPortIndex(const quint8 portIndex)
{
    auto portId = portIndex - 1;
    if (portId >= 0 && portId < validPorts().count())
        m_portIndex = portIndex;
    else
        m_portIndex = 0;

    Q_EMIT portIndexChanged();
}

/**
 * @brief Serial::setParity
 * @param parityIndex
 */
void Serial::setParity(const quint8 parityIndex)
{
    // Argument verification
    Q_ASSERT(parityIndex < parityList().count());

    // Update current index
    m_parityIndex = parityIndex;

    // Set parity based on current index
    switch (parityIndex)
    {
        case 0:
            m_parity = QSerialPort::NoParity;
            break;
        case 1:
            m_parity = QSerialPort::EvenParity;
            break;
        case 2:
            m_parity = QSerialPort::OddParity;
            break;
        case 3:
            m_parity = QSerialPort::SpaceParity;
            break;
        case 4:
            m_parity = QSerialPort::MarkParity;
            break;
    }

    // Update serial port config.
    if (port())
        port()->setParity(parity());

    // Notify user interface
    Q_EMIT parityChanged();
}

/**
 * Registers the new baud rate to the list
 */
void Serial::appendBaudRate(const QString &baudRate)
{
    if (!m_baudRateList.contains(baudRate))
    {
        m_baudRateList.append(baudRate);
        writeSettings();
        Q_EMIT baudRateListChanged();
    }
}

/**
 * Changes the data bits of the serial port.
 *
 * @note This function is meant to be used with a combobox in the
 *       QML interface
 */
void Serial::setDataBits(const quint8 dataBitsIndex)
{
    // Argument verification
    Q_ASSERT(dataBitsIndex < dataBitsList().count());

    // Update current index
    m_dataBitsIndex = dataBitsIndex;

    // Obtain data bits value from current index
    switch (dataBitsIndex)
    {
        case 0:
            m_dataBits = QSerialPort::Data5;
            break;
        case 1:
            m_dataBits = QSerialPort::Data6;
            break;
        case 2:
            m_dataBits = QSerialPort::Data7;
            break;
        case 3:
            m_dataBits = QSerialPort::Data8;
            break;
    }

    // Update serial port configuration
    if (port())
        port()->setDataBits(dataBits());

    // Update user interface
    Q_EMIT dataBitsChanged();
}

/**
 * Changes the stop bits of the serial port.
 *
 * @note This function is meant to be used with a combobox in the
 *       QML interface
 */
void Serial::setStopBits(const quint8 stopBitsIndex)
{
    // Argument verification
    Q_ASSERT(stopBitsIndex < stopBitsList().count());

    // Update current index
    m_stopBitsIndex = stopBitsIndex;

    // Obtain stop bits value from current index
    switch (stopBitsIndex)
    {
        case 0:
            m_stopBits = QSerialPort::OneStop;
            break;
        case 1:
            m_stopBits = QSerialPort::OneAndHalfStop;
            break;
        case 2:
            m_stopBits = QSerialPort::TwoStop;
            break;
    }

    // Update serial port configuration
    if (port())
        port()->setStopBits(stopBits());

    // Update user interface
    Q_EMIT stopBitsChanged();
}

/**
 * Enables or disables the auto-reconnect feature
 */
void Serial::setAutoReconnect(const bool autoreconnect)
{
    m_autoReconnect = autoreconnect;
    Q_EMIT autoReconnectChanged();
}

/**
 * Changes the flow control option of the serial port.
 *
 * @note This function is meant to be used with a combobox in the
 *       QML interface
 */
void Serial::setFlowControl(const quint8 flowControlIndex)
{
    // Argument verification
    Q_ASSERT(flowControlIndex < flowControlList().count());

    // Update current index
    m_flowControlIndex = flowControlIndex;

    // Obtain flow control value from current index
    switch (flowControlIndex)
    {
        case 0:
            m_flowControl = QSerialPort::NoFlowControl;
            break;
        case 1:
            m_flowControl = QSerialPort::HardwareControl;
            break;
        case 2:
            m_flowControl = QSerialPort::SoftwareControl;
            break;
    }

    // Update serial port configuration
    if (port())
        port()->setFlowControl(flowControl());

    // Update user interface
    Q_EMIT flowControlChanged();
}

/**
 * Scans for new serial ports available & generates a QStringList with current
 * serial ports.
 */
void Serial::refreshSerialDevices()
{
    // Create device list, starting with dummy header
    // (for a more friendly UI when no devices are attached)
    QStringList ports;
    ports.append(tr("Select port"));

    // Search for available ports and add them to the lsit
    auto validPortList = validPorts();
    Q_FOREACH (QSerialPortInfo info, validPortList)
    {
        if (!info.isNull())
            ports.append(info.portName());
    }

    // Update list only if necessary
    if (portList() != ports)
    {
        // Update list
        m_portList = ports;

        // Update current port index
        if (port())
        {
            auto name = port()->portName();
            for (int i = 0; i < validPortList.count(); ++i)
            {
                auto info = validPortList.at(i);
                if (info.portName() == name)
                {
                    m_portIndex = i + 1;
                    break;
                }
            }
        }

        // Auto reconnect
        if (autoReconnect() && m_lastSerialDeviceIndex > 0)
        {
            if (m_lastSerialDeviceIndex < portList().count())
            {
                setPortIndex(m_lastSerialDeviceIndex);
            }
        }

        // Update UI
        Q_EMIT availablePortsChanged();
    }
}

/**
 * @brief Serial::handleError
 * @param error
 */
void Serial::handleError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError)
    {
        qDebug() << error;
        disconnectDevice();
    }
}

/**
 * Reads all the data from the serial port & sends it to the @c IO::Manager class
 */
void Serial::onReadyRead()
{
    if (isOpen())
        Q_EMIT dataReceived(port()->readAll());
}

/**
 * Read saved settings (if any)
 */
void Serial::readSettings()
{
    // Register standard baud rates
    QStringList stdBaudRates
        = { "300",   "1200",   "2400",   "4800",   "9600",   "19200",   "38400",  "57600",
            "74880", "115200", "230400", "250000", "500000", "1000000", "2000000" };

    // Get value from settings
    QStringList list;
    list = m_settings.value("IO_DataSource_Serial__BaudRates", stdBaudRates)
               .toStringList();

    // Convert QStringList to QVector
    m_baudRateList.clear();
    for (int i = 0; i < list.count(); ++i)
        m_baudRateList.append(list.at(i));

        // Sort baud rate list
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    for (auto i = 0; i < m_baudRateList.count() - 1; ++i)
    {
        for (auto j = 0; j < m_baudRateList.count() - i - 1; ++j)
        {
            auto a = m_baudRateList.at(j).toInt();
            auto b = m_baudRateList.at(j + 1).toInt();
            if (a > b)
                m_baudRateList.swapItemsAt(j, j + 1);
        }
    }
#endif

    // Notify UI
    Q_EMIT baudRateListChanged();
}

/**
 * Save settings between application runs
 */
void Serial::writeSettings()
{
    // Sort baud rate list
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    for (auto i = 0; i < m_baudRateList.count() - 1; ++i)
    {
        for (auto j = 0; j < m_baudRateList.count() - i - 1; ++j)
        {
            auto a = m_baudRateList.at(j).toInt();
            auto b = m_baudRateList.at(j + 1).toInt();
            if (a > b)
            {
                m_baudRateList.swapItemsAt(j, j + 1);
                Q_EMIT baudRateListChanged();
            }
        }
    }
#endif

    // Convert QVector to QStringList
    QStringList list;
    for (int i = 0; i < baudRateList().count(); ++i)
        list.append(baudRateList().at(i));

    // Save list to memory
    m_settings.setValue("IO_DataSource_Serial__BaudRates", list);
}

/**
 * Returns a list with all the valid serial port objects
 */
QVector<QSerialPortInfo> Serial::validPorts() const
{
    // Search for available ports and add them to the list
    QVector<QSerialPortInfo> ports;
    Q_FOREACH (QSerialPortInfo info, QSerialPortInfo::availablePorts())
    {
        if (!info.isNull())
        {
            // Only accept *.cu devices on macOS (remove *.tty)
            // https://stackoverflow.com/a/37688347
#ifdef Q_OS_MACOS
            if (info.portName().toLower().startsWith("tty."))
                continue;
#endif
            // Append port to list
            ports.append(info);
        }
    }

    // Return list
    return ports;
}
