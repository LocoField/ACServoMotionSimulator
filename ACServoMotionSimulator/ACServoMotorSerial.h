#pragma once

#include "SerialPort.h"

class ACServoMotorSerial : public SerialPort
{
public:
	ACServoMotorSerial();
	ACServoMotorSerial(const ACServoMotorSerial& object);
	virtual ~ACServoMotorSerial();

protected:
	virtual int checkCompleteData(const std::vector<unsigned char>& data);

public:
	void setAddress(int address);
	bool connect(const std::string& port, int baudRate);
	void disconnect();

	virtual size_t write(const std::vector<unsigned char>& data);
	virtual std::vector<unsigned char> writeAndRead(const std::vector<unsigned char>& data);

	bool paramValue(unsigned char param, short& value);
	bool position(int& pos, bool& moving);

	bool setCycle(int cycle, unsigned char index);
	bool setSpeed(unsigned short speed, unsigned char index);
	bool setSpeed(unsigned short speed);
	void test(short value);

	void trigger(unsigned char index);
	void normal();

	void emergency(bool on);
	void power(bool on);
	void home();
	void stop();

protected:
	int address = 1;

};

