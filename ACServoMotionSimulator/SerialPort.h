#pragma once

#include <QtSerialPort/QSerialPort>

class SerialPort : protected QSerialPort
{
public:
	SerialPort();
	~SerialPort() = default;

public:
	static void availablePorts(std::vector<QString>& ports);

protected:
	virtual int checkCompleteData(const std::vector<unsigned char>& data) { return 0; }

public:
	bool connect(QString portName, int baudRate, QSerialPort::Parity parity, QSerialPort::StopBits stopBits);
	void disconnect();
	bool isConnected();

	void setDisconnectedCallback(std::function<void()> callback);

	qint64 write(const std::vector<unsigned char>& data);
	std::vector<unsigned char> read(int timeout = 2000);
	std::vector<unsigned char> writeAndRead(const std::vector<unsigned char>& data, int timeout = 2000);

private:
	std::function<void()> disconnectedCallback;

};

