#include "stdafx.h"
#include "ACServoMotorSerial.h"
#include "ACServoMotorHelper.h"

ACServoMotorSerial::ACServoMotorSerial()
{
}

ACServoMotorSerial::ACServoMotorSerial(const ACServoMotorSerial& object)
{
	this->address = object.address;
}

ACServoMotorSerial::~ACServoMotorSerial()
{
	disconnect();
}

int ACServoMotorSerial::checkCompleteData(const std::vector<unsigned char>& data)
{
	int expectedLength = ACServoMotorHelper::getDataLength(data);
	if (expectedLength > data.size())
		return -1;

	return expectedLength;
}

void ACServoMotorSerial::setAddress(int address)
{
	this->address = address;
}

bool ACServoMotorSerial::connect(const std::string& port, int baudRate)
{
	if (SerialPort::connect(port, baudRate, 8, 1, 1) == false)
	{
		printf("ERROR: motor connect failed.\n");

		return false;
	}

	short motionStationNumber = 0;
	bool paramCheck = readParam(65, motionStationNumber);

	if (paramCheck == false || motionStationNumber != address)
	{
		printf("ERROR: motor station number failed.\n");

		SerialPort::disconnect();
		return false;
	}

	return true;
}

void ACServoMotorSerial::disconnect()
{
	SerialPort::disconnect();
}

size_t ACServoMotorSerial::write(const std::vector<unsigned char>& data)
{
	printf(">> ");

	for (auto& d : data)
		printf("%d ", d);

	printf("\n");

	return SerialPort::write(data);
}

std::vector<unsigned char> ACServoMotorSerial::writeAndRead(const std::vector<unsigned char>& data)
{
	write(data);
	size_t expectedAnswerLength = ACServoMotorHelper::getDataLength(data);

	std::vector<unsigned char> received;
	serial.read(received, expectedAnswerLength);

	printf("<< ");

	for (auto& r : received)
		printf("%d ", r);

	printf("\n");

	return received;
}

bool ACServoMotorSerial::readParam(unsigned char param, short& value)
{
	auto received = writeAndRead(ACServoMotorHelper::readParam(address, param));

	if (ACServoMotorHelper::getParamValue(received, value) == false)
		return false;

	return true;
}

bool ACServoMotorSerial::setParam(unsigned char param, short value)
{
	auto received = writeAndRead(ACServoMotorHelper::setParam(address, param, value));

	return true;
}

bool ACServoMotorSerial::position(int& pos, bool& moving)
{
	auto received = writeAndRead(ACServoMotorHelper::readEncoder(address));

	if (ACServoMotorHelper::getEncoderValue(received, pos, moving) == false)
		return false;

	return true;
}

bool ACServoMotorSerial::setCycle(int cycle, unsigned char index)
{
	writeAndRead(ACServoMotorHelper::setCycle(address, cycle, index));

	return true;
}

bool ACServoMotorSerial::setSpeed(unsigned short speed, unsigned char index)
{
	writeAndRead(ACServoMotorHelper::setSpeed(address, speed, index));

	return true;
}

bool ACServoMotorSerial::setSpeed(unsigned short speed)
{
	writeAndRead(ACServoMotorHelper::setSpeed(address, speed));

	return true;
}

void ACServoMotorSerial::trigger(unsigned char index)
{
	writeAndRead(ACServoMotorHelper::trigger(address, index));
}

void ACServoMotorSerial::normal()
{
	writeAndRead(ACServoMotorHelper::normal(address));
}

void ACServoMotorSerial::power(bool on)
{
	writeAndRead(ACServoMotorHelper::power(address, on));
}

void ACServoMotorSerial::home()
{
	writeAndRead(ACServoMotorHelper::home(address));
	writeAndRead(ACServoMotorHelper::normal(address));
}

void ACServoMotorSerial::stop()
{
	writeAndRead(ACServoMotorHelper::stop(address));
}
