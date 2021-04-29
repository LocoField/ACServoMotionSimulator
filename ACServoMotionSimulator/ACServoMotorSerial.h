#pragma once

#include "SerialPort.h"

class ACServoMotorSerial : protected SerialPort
{
public:
	ACServoMotorSerial() = default;
	~ACServoMotorSerial() = default;

protected:
	virtual int checkCompleteData(const std::vector<unsigned char>& data);

public:
	bool connect(QString portName, int baudRate, int numMotors);
	void disconnect();

	void setDisconnectedCallback(std::function<void()> callback);

	bool paramValue(int device, int param, short& value);
	bool position(int device, int& pos);
	void wait(int timeout = 2000);

	bool setCycle(int cycle, int index, int device = -1);
	bool setSpeed(int speed, int device = -1);

	bool power(bool on, int device = -1);
	bool home(int device = -1);
	bool stop(int device = -1);
	bool trigger(int index, int device = -1);

	void emergency(bool on);

protected:
	int numMotors_ = 0;

};

