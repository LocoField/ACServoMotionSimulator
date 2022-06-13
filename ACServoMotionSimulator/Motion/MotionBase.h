#pragma once

#define _USE_MATH_DEFINES

#include <math.h>

struct Motion
{
	float roll = 0;
	float pitch = 0;
	float heave = 0;
	float yaw = 0;
	float sway = 0;
	float surge = 0;

	float ll = 0;
	float lr = 0;
	float rl = 0;
	float rr = 0;
};

class MotionBase
{
public:
	MotionBase() = default;
	virtual ~MotionBase() = default;

public:
	virtual char* getMotionName() abstract;

	virtual void motion(Motion& motion) { motion = motion_; }

	virtual bool start() { return true; }
	virtual void stop() {}

	virtual bool process(void* arg) abstract;

protected:
	Motion motion_;

};
