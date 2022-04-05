#pragma once

#include "ACServoMotorSerial.h"

#include <QtWidgets/QDialog>

#include <shared_mutex>

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

	std::vector<ACServoMotorSerial> motors;
	std::vector<int> currentPositions;

	std::vector<ACServoMotionBase*> motionSources;
	ACServoMotionBase* motionSource = nullptr;
	std::map<QString, QJsonObject> motionOptions;

	int angle = 5000; // difference
	int width = 500;
	int height = 1000;
	int baudRate = 115200;
	int center = 0;
	int limit = 0;
	int numMotors = 0;
	QString portNames;
	int sign = 1; // 1 or -1
	int speed = 1000; // rpm

};
