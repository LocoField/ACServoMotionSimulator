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
	bool succeed = true;

	motors_.reserve(numMotors);

	for (int i = 0; i < numMotors; i++)
	{
		SerialPort* motor = new SerialPort;

		if (motor->connect(portName, baudRate, QSerialPort::OddParity, QSerialPort::OneStop) == false)
		{
			printf("ERROR: motor connect failed.\n");

			delete motor;
			succeed = false;
			break;
		}

		short motionStationNumber = 0;
		bool paramCheck = paramValue(i, 65, motionStationNumber);

		if (paramCheck == false || motionStationNumber != i)
		{
			printf("ERROR: motor station number failed.\n");

			delete motor;
			succeed = false;
			break;
		}

		motors_.emplace_back(motor);
	}

	if (succeed == false)
	{
		disconnect();
		return false;
	}

	return true;
}

void ACServoMotorSerial::disconnect()
{
	for (int i = 0; i < motors_.size(); i++)
	{
		delete motors_[i];
	}

	motors_.clear();
}

void ACServoMotorSerial::setDisconnectedCallback(std::function<void()> callback)
{
	for (int i = 0; i < motors_.size(); i++)
	{
		motors_[i]->setDisconnectedCallback(callback);
	}
}

bool ACServoMotorSerial::paramValue(int device, unsigned char param, short& value)
{
	if (device < 0 || motors_.size() <= device) return false;

	auto received = motors_[device]->writeAndRead(ACServoMotorHelper::readParam(device, param));

	if (ACServoMotorHelper::getParamValue(received, value) == false)
		return false;

	return true;
}

bool ACServoMotorSerial::position(int device, int& pos)
{
	if (device < 0 || motors_.size() <= device) return false;

	bool moving = true;

	auto received = motors_[device]->writeAndRead(ACServoMotorHelper::readEncoder(device));

	if (ACServoMotorHelper::getEncoderValue(received, pos, moving) == false)
		return false;

	if (moving)
		return false;

	return true;
}

bool ACServoMotorSerial::setCycle(int device, int cycle, unsigned char index)
{
	if (device < 0 || motors_.size() <= device) return false;

	motors_[device]->writeAndRead(ACServoMotorHelper::setCycle(device, cycle, index));

	return true;
}

bool ACServoMotorSerial::setSpeed(int device, unsigned short speed)
{
	if (device < 0 || motors_.size() <= device) return false;

	motors_[device]->writeAndRead(ACServoMotorHelper::setSpeed(device, speed));

	return true;
}

void ACServoMotorSerial::emergency(bool on)
{
	for (int i = 0; i < motors_.size(); i++)
	{
		auto command = ACServoMotorHelper::emergency(i, on);

		motors_[i]->write({ (char*)command.data(), (int)command.size() });
	}

	for (int i = 0; i < motors_.size(); i++)
	{
		motors_[i]->read();
	}
}

bool ACServoMotorSerial::power(bool on)
{
	for (int i = 0; i < motors_.size(); i++)
	{
		motors_[i]->writeAndRead(ACServoMotorHelper::power(i, on));
	}

	return true;
}

bool ACServoMotorSerial::home()
{
	for (int i = 0; i < motors_.size(); i++)
	{
		motors_[i]->writeAndRead(ACServoMotorHelper::home(i));
		motors_[i]->writeAndRead(ACServoMotorHelper::normal(i));
	}

	Sleep(1000);

	return true;
}

bool ACServoMotorSerial::stop()
{
	for (int i = 0; i < motors_.size(); i++)
	{
		motors_[i]->writeAndRead(ACServoMotorHelper::stop(i));
	}

	return true;
}

bool ACServoMotorSerial::trigger(int device, unsigned char index)
{
	if (motors_.size() <= device) return false;

	motors_[device]->writeAndRead(ACServoMotorHelper::trigger(device, index));
	motors_[device]->writeAndRead(ACServoMotorHelper::normal(device));

	return true;
}

void ACServoMotorSerial::wait(int timeout)
{
	timeout /= 100;

	for (int i = 0; i < timeout; i++)
	{
		bool allMoved = true;

		for (int i = 0; i < motors_.size(); i++)
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
