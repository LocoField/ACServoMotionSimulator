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
		printf("ERROR: motor connect failed.\n");

		return false;
	}

	bool succeed = true;

	for (int i = 0; i < numMotors; i++)
	{
		short motionStationNumber = 0;

		bool paramCheck = paramValue(i, 65, motionStationNumber);

		if (paramCheck == false || motionStationNumber != i)
		{
			succeed = false;
			break;
		}
	}

	if (succeed == false)
	{
		printf("ERROR: motor station number failed.\n");

		__super::disconnect();
		return false;
	}

	numMotors_ = numMotors;

	return true;
}

void ACServoMotorSerial::disconnect()
{
	__super::disconnect();

	numMotors_ = 0;
}

void ACServoMotorSerial::setDisconnectedCallback(std::function<void()> callback)
{
	__super::setDisconnectedCallback(callback);
}

bool ACServoMotorSerial::paramValue(int device, int param, short& value)
{
	auto received = writeAndRead(ACServoMotorHelper::readParam(device + 1, param));

	if (ACServoMotorHelper::getParamValue(received, value) == false)
		return false;

	return true;
}

bool ACServoMotorSerial::position(int device, int& pos)
{
	bool moving = true;

	auto received = writeAndRead(ACServoMotorHelper::readEncoder(device + 1));

	if (ACServoMotorHelper::getEncoderValue(received, pos, moving) == false)
		return false;

	if (moving)
		return false;

	return true;
}

void ACServoMotorSerial::wait(int timeout)
{
	timeout /= 100;

	for (int i = 0; i < timeout; i++)
	{
		bool allMoved = true;

		for (int i = 0; i < numMotors_; i++)
		{
			int pos = 0;

			if (position(i, pos) == false)
			{
				allMoved = false;
				break;
			}
		}

		if (allMoved)
			break;

		Sleep(100);
	}
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

void ACServoMotorSerial::emergency(bool on)
{
	for (int i = 0; i < numMotors_; i++)
	{
		auto command = ACServoMotorHelper::emergency(on, i + 1);

		write({ (char*)command.data(), (int)command.size() });
		Sleep(10);
	}

	readAll();
}
