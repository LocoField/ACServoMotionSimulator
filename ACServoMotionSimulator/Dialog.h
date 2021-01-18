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

	std::vector<int> currentPositions;

	int angle = 5000; // difference
	int center = 0;
	int limit = 0;
	int numMotors = 0;
	QString portName;
	int sign = 1; // 1 or -1
	int speed = 1000; // rpm

};
