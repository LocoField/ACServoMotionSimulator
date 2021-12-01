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
	// ASCII: start(1) + address(2) + command(2) + data(?) + LRC(2) + CRLF(2) = 5 + ? + 4
	// RTU: address(1) + command(1) + data(?) + CRC(2) = 2 + ? + 2

	const size_t prefixLength = 2;
	const size_t postfixLength = 2;

	if (data.size() < prefixLength)
		return -1;

	unsigned char command = data[1];
	unsigned char dataSize = 0;

	switch (command)
	{
		case 0x03: // read multiple
		{
			if (data.size() == 8) // send data
				dataSize = (data[4] * 16 + data[5]) * 2 + 1;
			else
				dataSize = data[2] + 1;

			break;
		}
		case 0x06: // write single
		case 0x10: // write multiple
		{
			dataSize = 4;
			break;
		}
		default:
		{
			return -1;
		}
	}

	return prefixLength + dataSize + postfixLength;
}

bool ACServoMotorHelper::getParamValue(const Command& data, short& value)
{
	if (data.size() != getDataLength(data))
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
	if (data.size() != getDataLength(data))
		return false;

	if (data[1] != 0x03 || data[2] != 0x06) // check command and size
		return false;

	Command command(data.cbegin(), data.cend() - 2);
	calculateCRC(command);

	if (command != data)
		return false;

	short output = (data[3] << 8) | data[4];
	moving = (output & 8) != 0;

	short encoder_acc1 = (data[5] << 8) | data[6];
	short encoder_acc2 = (data[7] << 8) | data[8];

	position = encoder_acc1 + encoder_acc2 * 10000;
	return true;
}

Command ACServoMotorHelper::readParam(int address, unsigned char param)
{
	if (235 < param) return Command();

	Command data;
	data.reserve(8);

	data.insert(data.end(), {
		(unsigned char)address, 0x03, 0x00, param, 0x00, 0x01,
	});

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::readCycles(int address, unsigned char index)
{
	if (index < 0 || 3 < index) return Command();

	Command data;
	data.reserve(8);

	data.insert(data.end(), {
		(unsigned char)address, 0x03, 0x00, (unsigned char)(120 + index * 2), 0x00, 0x02,
	}); // Pn120 and Pn121 ~

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::readEncoder(int address)
{
	Command data;
	data.reserve(8);
	
	data.insert(data.end(), {
		(unsigned char)address, 0x03, 0x01, 0x82, 0x00, 0x03,
	}); // Dn018 to Dn020

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::setParam(int address, unsigned char param, short value)
{
	if (235 < param) return Command();

	Command data;
	data.reserve(8);

	data.insert(data.end(), {
		(unsigned char)address, 0x06, 0x00, param,
		(unsigned char)((value >> 8) & 0xFF), (unsigned char)(value & 0xFF),
	});

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::setCycle(int address, int cycle, unsigned char index)
{
	if (index < 0 || 3 < index) return Command();

	Command data;
	data.reserve(11);

	short high = cycle / 10000;
	short low = cycle % 10000;

	data.insert(data.end(), {
		(unsigned char)address, 0x10, 0x00, (unsigned char)(120 + index * 2), 0x00, 0x02, 0x04,
		(unsigned char)((high >> 8) & 0xFF), (unsigned char)(high & 0xFF),
		(unsigned char)((low >> 8) & 0xFF), (unsigned char)(low & 0xFF),
	});

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::setSpeed(int address, unsigned short speed, unsigned char index)
{
	if (speed < 0 || 3000 < speed) return Command();

	Command data;
	data.reserve(8);

	data.insert(data.end(), {
		(unsigned char)address, 0x06, 0x00, (unsigned char)(128 + index),
		(unsigned char)((speed >> 8) & 0xFF), (unsigned char)(speed & 0xFF),
	});

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::setSpeed(int address, unsigned short speed)
{
	if (speed < 0 || 3000 < speed) return Command();

	Command data;
	data.reserve(17);

	data.insert(data.end(), {
		(unsigned char)address, 0x10, 0x00, 128, 0x00, 0x04, 0x08,
		(unsigned char)((speed >> 8) & 0xFF), (unsigned char)(speed & 0xFF),
		(unsigned char)((speed >> 8) & 0xFF), (unsigned char)(speed & 0xFF),
		(unsigned char)((speed >> 8) & 0xFF), (unsigned char)(speed & 0xFF),
		(unsigned char)((speed >> 8) & 0xFF), (unsigned char)(speed & 0xFF),
	});

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::home(int address)
{
	std::bitset<16> pn71 = 0x7fff;
	pn71.set(6, false);

	auto value = pn71.to_ulong();

	Command data;
	data.reserve(8);

	data.insert(data.end(), {
		(unsigned char)address, 0x06, 0x00, 71,
		(unsigned char)((value >> 8) & 0xFF), (unsigned char)(value & 0xFF),
	});

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::stop(int address)
{
	std::bitset<16> pn71 = 0x7fff;
	pn71.set(11, false);

	auto value = pn71.to_ulong();

	Command data;
	data.reserve(8);

	data.insert(data.end(), {
		(unsigned char)address, 0x06, 0x00, 71,
		(unsigned char)((value >> 8) & 0xFF), (unsigned char)(value & 0xFF),
	});

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::trigger(int address, unsigned char index)
{
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
			return Command();
		}
	}

	auto value = pn71.to_ulong();

	Command data;
	data.reserve(8);

	data.insert(data.end(), {
		(unsigned char)address, 0x06, 0x00, 71,
		(unsigned char)((value >> 8) & 0xFF), (unsigned char)(value & 0xFF),
	});

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::normal(int address)
{
	std::bitset<16> pn71 = 0x7fff;
	auto value = pn71.to_ulong();

	Command data;
	data.reserve(8);

	data.insert(data.end(), {
		(unsigned char)address, 0x06, 0x00, 71,
		(unsigned char)((value >> 8) & 0xFF), (unsigned char)(value & 0xFF),
	});

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::emergency(int address, bool on)
{
	Command data;
	data.reserve(8);

	data.insert(data.end(), {
		(unsigned char)address, 0x06, 0x00, 70, 0x7F,
	});

	if (on) data.push_back(0xFF);
	else data.push_back(0xBF);

	calculateCRC(data);
	return data;
}

Command ACServoMotorHelper::power(int address, bool on)
{
	Command data;
	data.reserve(8);

	data.insert(data.end(), {
		(unsigned char)address, 0x06, 0x00, 70, 0x7F,
		});

	if (on) data.push_back(0xBE);
	else data.push_back(0xBF);

	calculateCRC(data);
	return data;
}
