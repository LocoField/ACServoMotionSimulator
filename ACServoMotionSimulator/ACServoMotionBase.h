#pragma once

class Vector3
{
public:
	float x = 0;
	float y = 0;
	float z = 0;

	bool operator==(const Vector3& other)
	{
		return abs(x - other.x) < 0.1f && abs(y - other.y) < 0.1f && abs(z - other.z) < 0.1f;
	}
};

class ACServoMotionBase
{
public:
	ACServoMotionBase() = default;
	virtual ~ACServoMotionBase() = default;

public:
	virtual char* getMotionName() abstract;
	virtual int getWaitTime() { return 10; }

	virtual void position(Vector3& angle) { angle = this->angle; }

	virtual bool start() { return true; }
	virtual void stop() {}

	virtual bool process(void* arg) abstract;

protected:
	Vector3 angle;

};
