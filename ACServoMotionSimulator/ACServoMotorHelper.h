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

	static bool getEncoderValue(const Command& data, int& position, bool& moving); // high + low * 10000 bits

	static Command readCycles(int address, int index);
	static Command readEncoder(int address);

	static Command setCycle(int cycle, int address, int index);
	static Command setSpeed(int speed, int address);

	static Command power(bool on, int address);
	static Command stop(int address, int index);
	static Command trigger(int address, int index);
	static Command normal(int address);

	static Command emergency(bool on, int address);

};
