#pragma once

#define _USE_MATH_DEFINES

#include <math.h>

struct Vector3
{
	float x = 0;
	float y = 0;
	float z = 0;
};

struct Vector4
{
	float ll = 0;
	float lr = 0;
	float rl = 0;
	float rr = 0;
};

class ACServoMotionBase
{
public:
	ACServoMotionBase() = default;
	virtual ~ACServoMotionBase() = default;

public:
	virtual char* getMotionName() abstract;

	virtual void angle(Vector3& angle) { angle = angle_; }
	virtual void axis(Vector4& axis) { axis = axis_; }

	virtual bool start() { return true; }
	virtual void stop() {}

	virtual bool process(void* arg) abstract;

protected:
	Vector3 angle_; // roll pitch yaw
	Vector4 axis_; // axis

};
