#pragma once

#include "SerialPort.h"

class ACServoMotorSerial : public SerialPort
{
public:
	ACServoMotorSerial() = default;
	~ACServoMotorSerial() = default;

protected:
	virtual int checkCompleteData(const std::vector<unsigned char>& data);

public:
	bool connect(QString portName, int baudRate, int numMotors);

	bool setCycle(int position, int index, int device = -1);
	bool setSpeed(int speed, int device = -1);

	bool power(bool on, int device = -1);
	bool stop(int index, int device = -1);
	bool trigger(int index, int device = -1);

	bool position(int index, int& position, bool& moving);

protected:
	int numMotors_ = 0;

};

