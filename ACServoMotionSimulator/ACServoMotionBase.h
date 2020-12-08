#pragma once

struct Axis
{
	float roll = 0;
	float pitch = 0;
	float yaw = 0;
};

class ACServoMotionBase
{
public:
	ACServoMotionBase() = default;
	virtual ~ACServoMotionBase() = default;

public:
	virtual char* getMotionName() abstract;
	virtual int getWaitTime() { return 10; }

	virtual void position(Axis& axis) { axis = this->axis; }

	virtual bool start() { return true; }
	virtual void stop() {}

	virtual bool process(void* arg) abstract;

protected:
	Axis axis;

};
