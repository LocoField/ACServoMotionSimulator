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
	static bool getTorqueValue(const Command& data, short& torque);
	static bool getEncoderValue(const Command& data, int& position, bool& moving); // high + low * 10000 bits

	static Command readParam(int address, unsigned char param);
	static Command readCycles(int address, unsigned char index);
	static Command readTorque(int address);
	static Command readEncoder(int address);

	static Command setParam(int address, unsigned char param, short value);
	static Command setCycle(int address, int cycle, unsigned char index);
	static Command setSpeed(int address, unsigned short speed, unsigned char index);
	static Command setSpeed(int address, unsigned short speed);

	static Command home(int address);
	static Command stop(int address);
	static Command trigger(int address, unsigned char index);
	static Command normal(int address);

	static Command power(int address, bool on);

};
