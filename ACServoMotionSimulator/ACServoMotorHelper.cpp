#include "ACServoMotorHelper.h"

#include <bitset>

void ACServoMotorHelper::calculateCRC(Command& data)
{
	unsigned int crc_reg = 0xFFFF;

	unsigned char* pdata = data.data();
	int length = (int)data.size();

	while (length--)
	{
		crc_reg ^= *(pdata++);

		for (int j = 0; j < 8; j++)
		{
			if (crc_reg & 0x01)
			{
				crc_reg = (crc_reg >> 1) ^ 0xA001;
			}
			else
			{
				crc_reg = (crc_reg >> 1);
			}
		}
	}

	data.push_back(crc_reg & 0xFF);
	data.push_back((crc_reg >> 8) & 0xFF);
}

int ACServoMotorHelper::getDataLength(const Command& data)
{
	const unsigned char minimumLength = 4; // mimimum length: address + command + [length] + CRC + CRC
	if (data.size() < minimumLength)
		return -1;

	unsigned char command = data[1];
	unsigned char dataSize = 0;

	switch (command)
	{
		case 0x03:
		{
			dataSize = data[2] + 1;
			break;
		}
		case 0x06:
		case 0x10:
		{
			// write
			dataSize = 4;
			break;
		}
		default:
		{
			return -1;
		}
	}

	return minimumLength + dataSize;
}

bool ACServoMotorHelper::checkRegistersWritten(const Command& data)
{
	if (data.size() < 8)
		return false;

	if (data[1] != 0x10)
		return false;

	Command command(data.cbegin(), data.cend() - 2);
	calculateCRC(command);

	if (command != data)
		return false;

	return true;
}

bool ACServoMotorHelper::getCycleValue(const Command& data, int& cycle)
{
	if (data.size() < 9)
		return false;

	if (data[1] != 0x03 || data[2] != 0x04) // check command and size
		return false;

	Command command(data.cbegin(), data.cend() - 2);
	calculateCRC(command);

	if (command != data)
		return false;

	int cycle_high = (data[3] << 8) | data[4];
	int cycle_low = (data[5] << 8) | data[6];

	cycle = cycle_high * 10000 + cycle_low;
	return true;
}

bool ACServoMotorHelper::getParamValue(const Command& data, int& value)
{
	if (data.size() < 7)
		return false;

	if (data[1] != 0x03 || data[2] != 0x02) // check command and size
		return false;

	Command command(data.cbegin(), data.cend() - 2);
	calculateCRC(command);

	if (command != data)
		return false;

	value = (data[3] << 8) | data[4];
	return true;
}

bool ACServoMotorHelper::getEncoderValue(const Command& data, int& position, bool& moving)
{
	if (data.size() < 11)
		return false;

	if (data[1] != 0x03 || data[2] != 0x06) // check command and size
		return false;

	Command command(data.cbegin(), data.cend() - 2);
	calculateCRC(command);

	if (command != data)
		return false;

	short output = (data[3] << 8) | data[4];
	moving = (output & 8) != 0;

	short encoder_high = (data[5] << 8) | data[6];
	short encoder_low = (data[7] << 8) | data[8];

	position = encoder_high + encoder_low * 10000;
	return true;
}

Command ACServoMotorHelper::readCycles(int address, int index)
{
	Command data;

	if (index < 0 || 3 < index)
		return data;

	data = { (unsigned char)address, 0x03, 0x00, (unsigned char)(120 + index * 2), 0x00, 0x02 }; // Pn120 and Pn121 ~

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::readGear(int address)
{
	Command data = { (unsigned char)address, 0x03, 0x00, 98, 0x00, 0x01 }; // Pn98: electron gear ratio

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::readEncoder(int address)
{
	Command data = { (unsigned char)address, 0x03, 0x01, 0x82, 0x00, 0x03 }; // Dn018 to Dn020

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::setPosition(int cycle, int address, int index)
{
	Command data;

	if (index < 0 || 3 < index)
		return data;

	short high = cycle / 10000;
	short low = cycle % 10000;

	data = { (unsigned char)address, 0x10, 0x00, (unsigned char)(120 + index * 2), 0x00, 0x02, 0x04 };
	data.push_back((high >> 8) & 0xFF);
	data.push_back((high & 0xFF));
	data.push_back((low >> 8) & 0xFF);
	data.push_back((low & 0xFF));

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::setSpeed(int speed, int address, int index)
{
	Command data;

	if (index < 0 || 3 < index)
		return data;

	if (speed < 0 || 3000 < speed)
		return data;

	data = { (unsigned char)address, 0x06, 0x00, (unsigned char)(128 + index), (unsigned char)((speed >> 8) & 0xFF), (unsigned char)(speed & 0xFF) };

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::stop(int address, int index)
{
	Command data;

	std::bitset<16> pn71 = 0x7fff;
	pn71.set(11, false);

	switch (index)
	{
		case 0:
		{
			pn71.set(8, true);
			pn71.set(9, true);
			break;
		}
		case 1:
		{
			pn71.set(8, false);
			pn71.set(9, true);
			break;
		}
		case 2:
		{
			pn71.set(8, true);
			pn71.set(9, false);
			break;
		}
		case 3:
		{
			pn71.set(8, false);
			pn71.set(9, false);
			break;
		}
		default:
		{
			return data;
		}
	}

	auto value = pn71.to_ulong();
	data = { (unsigned char)address, 0x06, 0x00, 71, (unsigned char)((value >> 8) & 0xFF), (unsigned char)(value & 0xFF) };

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::trigger(int address, int index)
{
	Command data;

	std::bitset<16> pn71 = 0x7fff;
	pn71.set(10, false);

	switch (index)
	{
		case 0:
		{
			pn71.set(8, true);
			pn71.set(9, true);
			break;
		}
		case 1:
		{
			pn71.set(8, false);
			pn71.set(9, true);
			break;
		}
		case 2:
		{
			pn71.set(8, true);
			pn71.set(9, false);
			break;
		}
		case 3:
		{
			pn71.set(8, false);
			pn71.set(9, false);
			break;
		}
		default:
		{
			return data;
		}
	}

	auto value = pn71.to_ulong();
	data = { (unsigned char)address, 0x06, 0x00, 71, (unsigned char)((value >> 8) & 0xFF), (unsigned char)(value & 0xFF) };

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::normal(int address)
{
	Command data;

	std::bitset<16> pn71 = 0x7fff;

	auto value = pn71.to_ulong();
	data = { (unsigned char)address, 0x06, 0x00, 71, (unsigned char)((value >> 8) & 0xFF), (unsigned char)(value & 0xFF) };

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::emergency(bool on, int address)
{
	Command data = { (unsigned char)address, 0x06, 0x00, 0x46, 0x7F };

	if (on) data.push_back(0xF2);
	else data.push_back(0xB2);

	calculateCRC(data);
	return data;
}
