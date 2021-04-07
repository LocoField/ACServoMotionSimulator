#pragma once

#include "ACServoMotionBase.h"

#include <QtGamepad/QGamepadKeyNavigation>

class ACServoMotionJoystick : public ACServoMotionBase
{
public:
	ACServoMotionJoystick() = default;
	virtual ~ACServoMotionJoystick() = default;

public:
	virtual char* getMotionName() override;

	virtual bool start();

	virtual bool process(void* arg);

private:
	QGamepadKeyNavigation joystick;

};
