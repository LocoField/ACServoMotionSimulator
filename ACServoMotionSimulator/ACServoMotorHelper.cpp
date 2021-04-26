#include "ACServoMotorHelper.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <bitset>

int fromASCIIHex(const unsigned char* c, int numBytes = 1)
{
	auto HexToDec = [](unsigned char value) -> int
	{
		value -= 48;
		if (value < 10) return value;
		else return value - 7;
	};

	int sum = 0;
	int offset = 0;

	for (int i = 0; i < numBytes; i++)
	{
		if (i != 0)
		{
			sum = (sum << 8);
		}

		sum += (HexToDec(c[offset])) * 16 + (HexToDec(c[offset + 1]));
		offset += 2;
	}

	return sum;
}

void ACServoMotorHelper::finalizeCommand(Command& data)
{
	int sum = 0;

	for (size_t i = 1; i < data.size(); i += 2)
	{
		sum += fromASCIIHex(&data[i]);
	}

	auto lrc = convertToHexString(-sum);

	data.reserve(data.size() + 4);
	data.push_back(lrc[2]);
	data.push_back(lrc[3]);
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
	ss << std::setfill('0') << std::setw(4) << std::hex << value;

	std::string str = ss.str();
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);

	if (str.length() > 4)
	{
		str = str.erase(0, 4);
	}

	return str;
}

int ACServoMotorHelper::getDataLength(const Command& data)
{
	// ASCII: start(1) + address(2) + command(2) + data(?) + LRC(2) + CRLF(2) = 9
	// RTU: address(1) + command(1) + data(?) + CRC(2) = 4

	const unsigned char minimumLength = 9;
	if (data.size() < minimumLength)
		return -1;

	unsigned char cmd = fromASCIIHex(&data[3]);
	unsigned char dataSize = 0;

	switch (cmd)
	{
		case 0x03:
		{
			dataSize = 2 + fromASCIIHex(&data[5]) * 2;
			break;
		}
		case 0x06:
		case 0x08:
		case 0x10:
		{
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

bool ACServoMotorHelper::getParamValue(const Command& data, int& value)
{
	if (data.size() != getDataLength(data))
		return false;

	unsigned char cmd = fromASCIIHex(&data[3]);
	if (cmd != 0x03)
		return false;

	Command command(data.cbegin(), data.cend() - 4);
	finalizeCommand(command);

	if (command != data)
		return false;

	value = fromASCIIHex(&data[7], 2);
	return true;
}

bool ACServoMotorHelper::getEncoderValue(const Command& data, int& position, bool& moving)
{
	if (data.size() != getDataLength(data))
		return false;

	unsigned char cmd = fromASCIIHex(&data[3]);
	if (cmd != 0x03)
		return false;

	Command command(data.cbegin(), data.cend() - 4);
	finalizeCommand(command);

	if (command != data)
		return false;

	short output = fromASCIIHex(&data[7], 2);
	moving = (output & 8) != 0;

	short encoder_high = fromASCIIHex(&data[11], 2);
	short encoder_low = fromASCIIHex(&data[15], 2);

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
	unsigned char value = 0;
	if (on) value = 'F';
	else value = 'B';

	Command data = {
		':', '0', convertToAddressByte(address),
		'0', '6',
		'0', '0', '4', '6',
		'7', 'F', value, '2',
	};

	finalizeCommand(data);
	return data;
}

Command ACServoMotorHelper::powerOn(bool on, int address)
{
	unsigned char value = 0;
	if (on) value = '1';
	else value = '0';

	Command data = {
		':', '0', convertToAddressByte(address),
		'0', '6',
		'0', '0', '0', '3',
		'0', '0', '0', value,
	};

	finalizeCommand(data);
	return data;
}
