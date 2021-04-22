#include "ACServoMotorHelper.h"

#include <sstream>
#include <bitset>

void ACServoMotorHelper::finalizeCommand(Command& data)
{
	unsigned char sum = 0;

	for (size_t i = 1; i < data.size(); i += 2)
	{
		sum += ((data[i] - 48) * 10 + (data[i + 1] - 48));
	}

	auto value = convertToHexString(-sum);

	data.reserve(data.size() + 4);
	data.push_back(value[0]);
	data.push_back(value[1]);
	data.push_back(0x0D);
	data.push_back(0x0A);
}

unsigned char ACServoMotorHelper::convertToAddressByte(int address)
{
	switch (address)
	{
		case 0: return '1';
		case 1: return '2';
		case 2: return '3';
		case 3: return '4';
	}

	return '\0';
}

std::string ACServoMotorHelper::convertToHexString(int value)
{
	std::stringstream ss;
	ss << std::hex << value;

	std::string str = ss.str();
	int padding = 4 - str.length(); // 0000

	if (padding < 0)
		return str.erase(0, 6);
	else if (padding > 0)
		return str.insert(0, padding, '0');
	else
		return str;
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

bool ACServoMotorHelper::getCycleValue(const Command& data, int& cycle)
{
	if (data.size() < 9)
		return false;

	if (data[1] != 0x03 || data[2] != 0x04) // check command and size
		return false;

	Command command(data.cbegin(), data.cend() - 4);
	finalizeCommand(command);

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

	Command command(data.cbegin(), data.cend() - 4);
	finalizeCommand(command);

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

	Command command(data.cbegin(), data.cend() - 4);
	finalizeCommand(command);

	if (command != data)
		return false;

	short output = (data[3] << 8) | data[4];
	moving = (output & 8) != 0;

	short encoder_high = (data[5] << 8) | data[6];
	short encoder_low = (data[7] << 8) | data[8];

	position = encoder_high + encoder_low * 10000;
	return true;
}

Command ACServoMotorHelper::readEncoder(int address)
{
	Command data;

	if (address < 0 || 3 < address) return data;

	data = {
		':', '0', convertToAddressByte(address),
		'0', '3',
		'0', '1', '8', '2',
		'0', '0', '0', '3',
	}; // Dn018 to Dn020 (Dn: 0170H~018CH)

	finalizeCommand(data);
	return data;
}

Command ACServoMotorHelper::setCycle(int cycle, int address, int index)
{
	Command data;

	if (address < 0 || 3 < address) return data;
	if (index < 0 || 3 < index) return data;

	short high = cycle / 10000;
	short low = cycle % 10000;

	auto param = convertToHexString(120 + index);
	auto value1 = convertToHexString(high);
	auto value2 = convertToHexString(low);

	data = {
		':', '0', convertToAddressByte(address),
		'1', '0',
		(unsigned char)param[0], (unsigned char)param[1], (unsigned char)param[2], (unsigned char)param[3],
		'0', '0', '0', '2',
		'0', '4',
		(unsigned char)value1[0], (unsigned char)value1[1], (unsigned char)value1[2], (unsigned char)value1[3],
		(unsigned char)value2[0], (unsigned char)value2[1], (unsigned char)value2[2], (unsigned char)value2[3],
	};

	finalizeCommand(data);
	return data;
}

Command ACServoMotorHelper::setSpeed(int speed, int address)
{
	Command data;

	if (address < 0 || 3 < address) return data;
	if (speed < 0 || 3000 < speed) return data;

	auto value = convertToHexString(speed);

	data = {
		':', '0', convertToAddressByte(address),
		'1', '0',
		'0', '0', '8', '0',
		'0', '0', '0', '3',
		'0', '8',
		(unsigned char)value[0], (unsigned char)value[1], (unsigned char)value[2], (unsigned char)value[3],
		(unsigned char)value[0], (unsigned char)value[1], (unsigned char)value[2], (unsigned char)value[3],
		(unsigned char)value[0], (unsigned char)value[1], (unsigned char)value[2], (unsigned char)value[3],
		(unsigned char)value[0], (unsigned char)value[1], (unsigned char)value[2], (unsigned char)value[3],
	};

	finalizeCommand(data);
	return data;
}

Command ACServoMotorHelper::stop(int address, int index)
{
	Command data;

	if (address < 0 || 3 < address) return data;

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

	auto value = convertToHexString((int)pn71.to_ulong());

	data = {
		':', '0', convertToAddressByte(address),
		'0', '6',
		'0', '0', '4', '7',
		(unsigned char)value[0], (unsigned char)value[1], (unsigned char)value[2], (unsigned char)value[3],
	};

	finalizeCommand(data);
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

	auto value = convertToHexString((int)pn71.to_ulong());

	data = {
		':', '0', convertToAddressByte(address),
		'0', '6',
		'0', '0', '4', '7',
		(unsigned char)value[0], (unsigned char)value[1], (unsigned char)value[2], (unsigned char)value[3],
	};

	finalizeCommand(data);
	return data;
}

Command ACServoMotorHelper::normal(int address)
{
	Command data;

	std::bitset<16> pn71 = 0x7fff;

	auto value = convertToHexString((int)pn71.to_ulong());

	data = {
		':', '0', convertToAddressByte(address),
		'0', '6',
		'0', '0', '4', '7',
		(unsigned char)value[0], (unsigned char)value[1], (unsigned char)value[2], (unsigned char)value[3],
	};

	finalizeCommand(data);
	return data;
}

Command ACServoMotorHelper::emergency(bool on, int address)
{
	Command data = {
		(unsigned char)address, 0x06, 0x00, 0x46, 0x7F };

	unsigned char value = 0;
	if (on) value = 'F';
	else value = 'B';

	data = {
		':', '0', convertToAddressByte(address),
		'0', '6',
		'0', '0', '4', '6',
		'7', 'F', value, '2',
	};

	finalizeCommand(data);
	return data;
}
