#pragma once

#include "MotionBase.h"

#include <QtCore/QObject>
#include <QtGui/QKeyEvent>

class MotionKeyboard : public MotionBase, public QObject
{
public:
	MotionKeyboard() = default;
	virtual ~MotionKeyboard() = default;

public:
	virtual char* getMotionName() override;

	virtual bool process(void* arg);

protected:
	bool eventFilter(QObject* object, QEvent* event) override;

protected:
	int pitchMoved = 0;
	int rollMoved = 0;

};
