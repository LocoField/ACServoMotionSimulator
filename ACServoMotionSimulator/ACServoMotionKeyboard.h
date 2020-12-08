#pragma once

#include "ACServoMotionBase.h"

class ACServoMotionKeyboard : public ACServoMotionBase, public QObject
{
public:
	ACServoMotionKeyboard() = default;
	virtual ~ACServoMotionKeyboard() = default;

public:
	virtual char* getMotionName() override;

	virtual bool process(void* arg);

protected:
	bool eventFilter(QObject* object, QEvent* event) override;

protected:
	int pitchMoved = 0;
	int rollMoved = 0;

};
