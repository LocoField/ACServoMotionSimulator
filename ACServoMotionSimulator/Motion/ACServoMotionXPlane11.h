#pragma once

#include "ACServoMotionBase.h"

#pragma pack(push, 1)
struct Data
{
	char header[4]; // DATA
	char padding[1];
	unsigned int r1;
	// pitch, roll, hding(true), hding(mag)
	float values[4];
};
#pragma pack(pop)

class ACServoMotionXPlane11 : public ACServoMotionBase
{
public:
	ACServoMotionXPlane11();
	virtual ~ACServoMotionXPlane11() = default;

public:
	virtual char* getMotionName() override;

	virtual bool start();
	virtual void stop();

	virtual bool process(void* arg);

protected:
	unsigned long long sock;

};
