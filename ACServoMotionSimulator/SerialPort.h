#pragma once

#include "serial.h"

#include <functional>

class SerialPort
{
public:
	SerialPort() = default;
	~SerialPort() = default;

protected:
	virtual int checkCompleteData(const std::vector<unsigned char>& data) { return 0; }

public:
	bool connect(const std::string& port, int baudRate, int byteSize, int parity, int stopBits);
	void disconnect();
	bool isConnected();

	void setDisconnectedCallback(std::function<void()> callback);

	virtual size_t write(const std::vector<unsigned char>& data);
	virtual std::vector<unsigned char> read();
	virtual std::vector<unsigned char> writeAndRead(const std::vector<unsigned char>& data);

protected:
	std::function<void()> disconnectedCallback;

	serial::Serial serial;

};

