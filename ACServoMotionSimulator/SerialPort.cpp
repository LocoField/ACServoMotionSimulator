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

	QObject::connect(this, &QSerialPort::readyRead, [this]()
	{
		QByteArray bytes = readAll();
		receivedData += bytes;

		int length = checkCompleteData({ receivedData.data(), receivedData.data() + receivedData.size() });
		if (length == -1)
			return;

		std::unique_lock<std::mutex> locker(receiveMutex);
		receivedQueue.push_back(receivedData);

		receivedData.clear();
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

void SerialPort::setDisconnectedCallback(std::function<void()> callback)
{
	disconnectedCallback = callback;
}

qint64 SerialPort::write(const std::vector<unsigned char>& data)
{
	return __super::write({ (char*)data.data(), (int)data.size() });
}

std::vector<unsigned char> SerialPort::read(int timeout)
{
	std::vector<unsigned char> received;
	int count = timeout / 10;

	for (int i = 0; i < count; i++)
	{
		std::unique_lock<std::mutex> locker(receiveMutex);

		if (receivedQueue.size() > 0)
		{
			auto& data = receivedQueue.front();
			received = { data.data(), data.data() + data.size() };

			receivedQueue.pop_front();
			break;
		}

		Sleep(10);
	}

	return received;
}

std::vector<unsigned char> SerialPort::writeAndRead(const std::vector<unsigned char>& data, int timeout)
{
	if (write(data) != data.size())
		return std::vector<unsigned char>();

	return read(timeout);
}
