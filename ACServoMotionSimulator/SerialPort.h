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

	std::vector<unsigned char> writeAndRead(const std::vector<unsigned char>& data);

	qint64 write(const QByteArray& data);
	QByteArray read(int timeout = 2000);

	bool write(char code);
	bool read(char& code, int timeout = 2000);

private:
	std::function<void()> disconnectedCallback;

};

