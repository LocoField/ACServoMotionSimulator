#pragma once

#include "SerialPort.h"

class ACServoMotorSerial
{
public:
	explicit ACServoMotorSerial();
	virtual ~ACServoMotorSerial();

protected:
	virtual int checkCompleteData(const std::vector<unsigned char>& data);

public:
	bool connect(const QString& portNames, int baudRate, int numMotors);
	void disconnect();
	void clear();

	void setDisconnectedCallback(std::function<void()> callback);

	bool paramValue(int device, unsigned char param, short& value);
	bool position(int device, int& pos);

	bool setCycle(int device, int cycle, unsigned char index);
	bool setSpeed(int device, unsigned short speed, unsigned char index);
	bool setSpeed(int device, unsigned short speed);

	bool trigger(int device, unsigned char index);
	bool normal(int device);

	void emergency(bool on);
	bool power(bool on);
	bool home();
	bool stop();
	void wait(int timeout = 2000);

protected:
	std::vector<SerialPort*> motors_;
	std::function<void()> disconnectedCallback_;

};

