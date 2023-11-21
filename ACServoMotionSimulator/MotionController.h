#pragma once

#include "Motion/MotionBase.h"

class QSerialPort;

class MotionController final
{
public:
	MotionController();
	~MotionController();

public:
	void setMotionSize(int width, int height);
	void setCenterPosition(int center);
	void setLimitPosition(int limit);

public:
	bool connect(const std::string& portName, int baudRate);
	void disconnect();
	bool power(bool on);
	bool start();
	bool stop();
	void motion(const Motion& data);

protected:
	QSerialPort* board = nullptr;

	int width = 500;
	int height = 1000;
	int angleStep = 10;
	int linearStep = 10;

	std::vector<int> lastPositions;

};
