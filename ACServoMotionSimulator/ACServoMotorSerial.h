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
	bool connect(QString portName, int numMotors);

	bool setSpeed(int speed, int index = -1);

	bool setPosition(int position, int index = -1, bool autoTrigger = true);
	bool trigger(int index = -1);
	bool stop(int index = -1);

	bool position(int index, int& position, bool& moving);

protected:
	int numMotors_ = 0;

};

