#include "stdafx.h"
#include "ACServoMotorSerial.h"
#include "ACServoMotorHelper.h"

int ACServoMotorSerial::checkCompleteData(const std::vector<unsigned char>& data)
{
	int expectedLength = ACServoMotorHelper::getDataLength(data);
	if (expectedLength > data.size())
		return -1;

	return expectedLength;
}

bool ACServoMotorSerial::connect(QString portName, int numMotors)
{
	if (__super::connect(portName, 57600, QSerialPort::OddParity, QSerialPort::OneStop) == false)
	{
		printf("ERROR: motor connect failed\n");

		return false;
	}

	for (int i = 0; i < numMotors; i++)
	{
		int pos = 0;
		bool moving = true;

		if (position(i, pos, moving) == false)
		{
			printf("ERROR: motor command failed\n");

			return false;
		}
	}

	numMotors_ = numMotors;

	return true;
}

bool ACServoMotorSerial::setSpeed(int speed, int index)
{
	writeAndRead(ACServoMotorHelper::setSpeed(speed, index + 1));

	return true;
}

bool ACServoMotorSerial::setPosition(int position, int index, bool autoTrigger)
{
	int begin = index;
	int end = index + 1;

	if (index < 0)
	{
		begin = 0;
		end = numMotors_;
	}

	for (int i = begin; i < end; i++)
	{
		auto received = writeAndRead(ACServoMotorHelper::setPosition(position, i + 1));
		if (received.empty())
			continue;

		if (autoTrigger)
		{
			trigger(i);
		}
	}

	return true;
}

bool ACServoMotorSerial::trigger(int index)
{
	int begin = index;
	int end = index + 1;

	if (index < 0)
	{
		begin = 0;
		end = numMotors_;
	}

	for (int i = begin; i < end; i++)
	{
		writeAndRead(ACServoMotorHelper::trigger(i + 1));
		writeAndRead(ACServoMotorHelper::normal(i + 1));
	}

	return true;
}

bool ACServoMotorSerial::stop(int index)
{
	int begin = index;
	int end = index + 1;

	if (index < 0)
	{
		begin = 0;
		end = numMotors_;
	}

	for (int i = begin; i < end; i++)
	{
		writeAndRead(ACServoMotorHelper::stop(i + 1));
	}

	return true;
}

bool ACServoMotorSerial::position(int index, int& position, bool& moving)
{
	auto received = writeAndRead(ACServoMotorHelper::readEncoder(index + 1));

	if (ACServoMotorHelper::getEncoderValue(received, position, moving) == false)
	{
//#ifdef _DEBUG
		printf("ERROR: getEncoderValue()\n");

		for (auto& c : received)
			printf("%x ", c);
		printf("\n\n");
//#endif

		return false;
	}

	return true;
}
