#include "ACServoMotionJoystick.h"

#include <QtGamepad/QGamepad>

char* ACServoMotionJoystick::getMotionName()
{
	return "Joystick";
}

bool ACServoMotionJoystick::start()
{
	auto gamepad = joystick.gamepad();

	if (gamepad->isConnected() == false)
		return false;

	joystick.setButtonL1Key(Qt::Key_PageUp);
	joystick.setButtonR1Key(Qt::Key_PageDown);

	return true;
}

bool ACServoMotionJoystick::process(void* arg)
{
	auto gamepad = joystick.gamepad();

	double x = gamepad->axisLeftX();
	double y = gamepad->axisLeftY();

	angle_.x = (float)x;
	angle_.y = (float)y;

	return true;
}
