#pragma once

#include <vector>

typedef std::vector<unsigned char> Command;

class ACServoMotorHelper
{
public:
	ACServoMotorHelper() = default;
	~ACServoMotorHelper() = default;

protected:
	static void calculateCRC(Command& data);

public:
	static int getDataLength(const Command& data);

	static bool getParamValue(const Command& data, short& value);
	static bool getEncoderValue(const Command& data, int& position, bool& moving); // high + low * 10000 bits

	static Command readParam(int address, unsigned char param);
	static Command readCycles(int address, unsigned char index);
	static Command readEncoder(int address);

	static Command setCycle(int address, int cycle, unsigned char index);
	static Command setSpeed(int address, unsigned short speed);

	static Command home(int address);
	static Command stop(int address);
	static Command trigger(int address, unsigned char index);
	static Command normal(int address);

	static Command emergency(int address, bool on);
	static Command power(int address, bool on);

};
