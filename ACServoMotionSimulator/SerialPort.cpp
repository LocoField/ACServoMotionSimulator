#include "stdafx.h"
#include "SerialPort.h"

SerialPort::SerialPort()
{
	QObject::connect(this, &QSerialPort::errorOccurred, [&](QSerialPort::SerialPortError error)
	{
		if (error == QSerialPort::SerialPortError::ResourceError)
		{
			if (disconnectedCallback)
				disconnectedCallback();

			printf("ERROR: device disconnected.\n");
		}
	});
}

void SerialPort::availablePorts(std::vector<QString>& ports)
{
	auto p = QSerialPortInfo::availablePorts();

	ports.clear();
	ports.resize(p.size());

	std::transform(p.cbegin(), p.cend(), ports.begin(), [](const QSerialPortInfo& info)
	{
		return info.portName();
	});
}

bool SerialPort::connect(QString portName, int baudRate, QSerialPort::Parity parity, QSerialPort::StopBits stopBits)
{
	setPortName(portName);
	setBaudRate(baudRate);
	setDataBits(QSerialPort::Data8);
	setParity(parity);
	setStopBits(stopBits);
	setFlowControl(QSerialPort::NoFlowControl);

	return __super::open(QIODevice::ReadWrite);
}

void SerialPort::disconnect()
{
	__super::close();
}

bool SerialPort::isConnected()
{
	return __super::isOpen();
}

qint64 SerialPort::write(const std::vector<unsigned char>& data)
{
#ifdef _DEBUG
	QString command;

	for (auto it = data.cbegin(); it != data.cend(); ++it)
	{
		unsigned char hex = *it;
		QString hex_format = QString(" %1").arg(hex, 2, 16, QLatin1Char('0'));

		command.append(hex_format);
	}

	cout << "-->" << command.toStdString() << endl;
#endif

	auto sent = __super::write({ (char*)data.data(), (int)data.size() });

	waitForBytesWritten();

	return sent;
}

std::vector<unsigned char> SerialPort::read(int timeout)
{
	if (isConnected())
	{
		if (waitForReadyRead(timeout))
		{
			QByteArray data = __super::readAll();
			return { data.data(), data.data() + data.size() };
		}
	}

	return std::vector<unsigned char>();
}

std::vector<unsigned char> SerialPort::writeAndRead(const std::vector<unsigned char>& data, int timeout)
{
	write(data);

	std::vector<unsigned char> received;

	while (1)
	{
		auto data = read(timeout);
		if (data.size() == 0)
			break;

		received.insert(received.end(), data.cbegin(), data.cend());

		int length = checkCompleteData(received);
		if (length == -1)
			continue;

		break;
	}

#ifdef _DEBUG
	QString command;

	for (auto it = received.cbegin(); it != received.cend(); ++it)
	{
		unsigned char hex = *it;
		QString hex_format = QString(" %1").arg(hex, 2, 16, QLatin1Char('0'));

		command.append(hex_format);
	}

	cout << "<--" << command.toStdString() << endl;
#endif

	return received;
}

void SerialPort::setDisconnectedCallback(std::function<void()> callback)
{
	disconnectedCallback = callback;
}
