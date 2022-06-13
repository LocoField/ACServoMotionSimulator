#pragma once

#include <shared_mutex>

struct Motion;
class ACServoMotorSerial;

class MotionController
{
public:
	MotionController() = default;
	~MotionController() = default;

public:
	void setStepValue(int step);
	void setSpeedValue(int speed);
	void setMotionSize(int width, int height);
	void setCenterPosition(int center);
	void setLimitPosition(int limit);
	void setReverseDirection(bool reverse);

public:
	bool connect(const std::string& ports, int baudRate);
	void disconnect();
	bool power(bool on);
	bool start();
	bool stop();

	void executeMotion(const Motion& data);

private:
	void motionThread(size_t index);

	std::shared_mutex motionMutex;
	std::condition_variable_any motionWaiter;

protected:
	std::vector<ACServoMotorSerial*> motors;
	std::vector<int> motionTriggers;
	std::vector<int> currentPositions;

	int step = 0;
	int speed = 0;
	int width = 500;
	int height = 1000;
	int center = 0;
	int limit = 0;
	int sign = 1;

};

