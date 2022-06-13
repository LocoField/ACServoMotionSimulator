#pragma once

#include "MotionController.h"

#include <QtWidgets/QDialog>

class QVBoxLayout;
class QTimer;
class MotionBase;

class Dialog : public QDialog
{
public:
	Dialog();
	virtual ~Dialog();

protected:
	void initialize();

public:
	bool loadOption();

	void addMotionModules();
	void clearMotionModules();

private:
	QVBoxLayout* mainLayout;
	QWidget* mainWidget;

	QTimer* motionTimer;
	MotionController motion;

	QString portNames;
	int baudRate = 115200;

	std::vector<MotionBase*> motionSources;
	MotionBase* motionSource = nullptr;
	std::map<QString, QJsonObject> motionOptions;

};
