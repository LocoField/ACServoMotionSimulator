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
	static bool checkRegistersWritten(const Command& data);

	static bool getCycleValue(const Command& data, int& cycle); // high + low * 10000 bits
	static bool getParamValue(const Command& data, int& value);
	static bool getEncoderValue(const Command& data, int& position, bool& moving); // high + low * 10000 bits

	static Command readCycles(int address = 1, int index = 0);
	static Command readGear(int address = 1); // TODO: refactoring
	static Command readEncoder(int address = 1);

	static Command setPosition(int cycle, int address = 1, int index = 0);
	static Command setSpeed(int speed, int address = 1, int index = 0);

	static Command stop(int address = 1, int index = 0);
	static Command trigger(int address = 1, int index = 0);
	static Command normal(int address = 1);

	static Command emergency(bool on, int address = 1);

};
