#pragma once

#include "ACServoMotorSerial.h"

#include <QtWidgets/QDialog>

class QVBoxLayout;
class QTimer;

class Dialog : public QDialog
{
public:
	Dialog();
	virtual ~Dialog();

protected:
	void initialize();
	void updateUI();

public:
	bool loadOption();
	bool saveOption();

private:
	bool eventFilter(QObject* object, QEvent* event) override;
	void keyPressEvent(QKeyEvent *) override;
	void closeEvent(QCloseEvent* event) override;

private:
	QVBoxLayout* mainLayout;
	QVBoxLayout* motorLayout;
	QVBoxLayout* controllerLayout;

	// instead of using Q_OBJECT
	QTimer* timerUpdateUI;
	bool needUpdateUI = false;

	ACServoMotorSerial motor;
	QString portName;

	int numMotors = 0;
	std::vector<int> centerPositions;
	std::vector<int> limitPositions;

	int angle = 10000; // difference
	int speed = 1000; // rpm
	int sign = 1; // 1 or -1

	// for keyboard
	int pitchMoved = 0;
	int rollMoved = 0;

};
