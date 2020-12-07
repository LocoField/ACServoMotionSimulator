#include "stdafx.h"
#include "SerialPort.h"

SerialPort::SerialPort()
{
	QObject::connect(this, &QSerialPort::errorOccurred, [&](QSerialPort::SerialPortError error)
	{
		if (error == QSerialPort::SerialPortError::ResourceError)
		{
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

std::vector<unsigned char> SerialPort::writeAndRead(const std::vector<unsigned char>& data)
{
	write({ (char*)data.data(), (int)data.size() });

	std::vector<unsigned char> received;

	while (1)
	{
		QByteArray bytes = read();
		if (bytes.isEmpty())
			break;

		received.insert(received.end(), bytes.cbegin(), bytes.cend());

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

qint64 SerialPort::write(const QByteArray& data)
{
	qint64 retval = __super::write(data);
	waitForBytesWritten();
	return retval;
}

QByteArray SerialPort::read(int timeout)
{
	if (waitForReadyRead(timeout))
	{
		return __super::readAll();
	}

	return QByteArray();
}

bool SerialPort::write(char code)
{
	bool re = putChar(code);
	waitForBytesWritten();
	return re;
}

bool SerialPort::read(char& code, int timeout)
{
	if (waitForReadyRead(timeout))
	{
		getChar(&code);
		return true;
	}

	return false;
}
