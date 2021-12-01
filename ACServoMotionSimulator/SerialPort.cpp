#include "stdafx.h"
#include "SerialPort.h"

using namespace serial;

bool SerialPort::connect(const std::string& port, int baudRate, int byteSize, int parity, int stopBits)
{
	serial.setPort(port);
	serial.setBaudrate(baudRate);
	serial.setBytesize((bytesize_t)byteSize);
	serial.setParity((parity_t)parity);
	serial.setStopbits((stopbits_t)stopBits);

	serial.open();
	
	return isConnected();
}

void SerialPort::disconnect()
{
	serial.close();
}

bool SerialPort::isConnected()
{
	return serial.isOpen();
}

size_t SerialPort::write(const std::vector<unsigned char>& data)
{
	auto sent = serial.write(data);
	return sent;
}

std::vector<unsigned char> SerialPort::read()
{
	const int timeout = 2000;
	std::vector<unsigned char> received;

	for (int i = 0; i < timeout / 10; i++)
	{
		if (i != 0) Sleep(10);

		std::vector<unsigned char> data;

		if (serial.read(data) == 0)
			continue;

		received.insert(received.end(), data.cbegin(), data.cend());

		int length = checkCompleteData(received);
		if (length == -1)
			continue;

		break;
	}

	return received;
}

std::vector<unsigned char> SerialPort::writeAndRead(const std::vector<unsigned char>& data)
{
	write(data);

	auto received = read();
	return received;
}

void SerialPort::setDisconnectedCallback(std::function<void()> callback)
{
	disconnectedCallback = callback;
}
