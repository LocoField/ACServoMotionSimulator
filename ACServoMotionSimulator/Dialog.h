#pragma once

#include "ACServoMotorSerial.h"

#include <QtWidgets/QDialog>

class QVBoxLayout;
class QTimer;
class ACServoMotionBase;

class Dialog : public QDialog
{
public:
	Dialog();
	virtual ~Dialog();

protected:
	void initialize();
	void updateUI(const std::vector<int>& positions);

public:
	bool loadOption();
	bool saveOption();

	void addMotionModules();

private:
	QVBoxLayout* mainLayout;
	QVBoxLayout* motorLayout;
	QVBoxLayout* controllerLayout;

	QTimer* motionTimer;

	std::vector<ACServoMotionBase*> motionSources;
	ACServoMotionBase* motionSource = nullptr;

	ACServoMotorSerial motor;

	int numMotors = 0;
	std::vector<int> centerPositions;
	std::vector<int> limitPositions;
	std::vector<int> currentPositions;

	QString portName;
	int angle = 5000; // difference
	int speed = 1000; // rpm
	int sign = 1; // 1 or -1

};
