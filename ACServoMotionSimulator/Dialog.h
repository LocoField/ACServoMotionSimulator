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

	void addMotionModules();
	void clearMotionModules();

private:
	QVBoxLayout* mainLayout;
	QWidget* mainWidget;

	QTimer* motionTimer;

	std::vector<ACServoMotionBase*> motionSources;
	ACServoMotionBase* motionSource = nullptr;
	std::map<QString, QJsonObject> motionOptions;

	ACServoMotorSerial motor;

	std::vector<int> currentPositions;

	int angle = 5000; // difference
	int baudRate = 115200;
	int center = 0;
	double gain = 1;
	int limit = 0;
	int numMotors = 0;
	QString portName;
	int sign = 1; // 1 or -1
	int speed = 1000; // rpm

};
