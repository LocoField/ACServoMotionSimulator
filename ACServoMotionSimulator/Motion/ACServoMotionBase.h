#pragma once

#define _USE_MATH_DEFINES

#include <math.h>

struct Vector3
{
	float x = 0;
	float y = 0;
	float z = 0;
};

class ACServoMotionBase
{
public:
	ACServoMotionBase() = default;
	virtual ~ACServoMotionBase() = default;

public:
	virtual char* getMotionName() abstract;

	virtual void position(Vector3& angle) { angle = this->angle; }

	virtual bool start() { return true; }
	virtual void stop() {}

	virtual bool process(void* arg) abstract;

protected:
	Vector3 angle;

};
