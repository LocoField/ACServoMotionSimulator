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

bool ACServoMotorSerial::connect(QString portName, int baudRate, int numMotors)
{
	if (__super::connect(portName, baudRate, QSerialPort::OddParity, QSerialPort::OneStop) == false)
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

bool ACServoMotorSerial::setCycle(int cycle, int index, int device)
{
	int begin = device;
	int end = device + 1;

	if (device < 0)
	{
		begin = 0;
		end = numMotors_;
	}

	for (int i = begin; i < end; i++)
	{
		writeAndRead(ACServoMotorHelper::setCycle(cycle, i + 1, index));
	}

	return true;
}

bool ACServoMotorSerial::setSpeed(int speed, int device)
{
	int begin = device;
	int end = device + 1;

	if (device < 0)
	{
		begin = 0;
		end = numMotors_;
	}

	for (int i = begin; i < end; i++)
	{
		writeAndRead(ACServoMotorHelper::setSpeed(speed, i + 1));
	}

	return true;
}

bool ACServoMotorSerial::power(bool on, int device)
{
	int begin = device;
	int end = device + 1;

	if (device < 0)
	{
		begin = 0;
		end = numMotors_;
	}

	for (int i = begin; i < end; i++)
	{
		writeAndRead(ACServoMotorHelper::power(on, i + 1));
	}

	return true;
}

bool ACServoMotorSerial::home(int device)
{
	int begin = device;
	int end = device + 1;

	if (device < 0)
	{
		begin = 0;
		end = numMotors_;
	}

	for (int i = begin; i < end; i++)
	{
		writeAndRead(ACServoMotorHelper::home(i + 1));
		writeAndRead(ACServoMotorHelper::normal(i + 1));
	}

	Sleep(1000);

	return true;
}

bool ACServoMotorSerial::stop(int device)
{
	int begin = device;
	int end = device + 1;

	if (device < 0)
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

bool ACServoMotorSerial::trigger(int index, int device)
{
	int begin = device;
	int end = device + 1;

	if (device < 0)
	{
		begin = 0;
		end = numMotors_;
	}

	for (int i = begin; i < end; i++)
	{
		writeAndRead(ACServoMotorHelper::trigger(i + 1, index));
		writeAndRead(ACServoMotorHelper::normal(i + 1));
	}

	return true;
}

bool ACServoMotorSerial::position(int device, int& pos, bool& moving)
{
	auto received = writeAndRead(ACServoMotorHelper::readEncoder(device + 1));

	if (ACServoMotorHelper::getEncoderValue(received, pos, moving) == false)
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
