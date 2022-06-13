#pragma once

#include "MotionBase.h"

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

class MotionXPlane11 : public MotionBase
{
public:
	MotionXPlane11();
	virtual ~MotionXPlane11() = default;

public:
	virtual char* getMotionName() override;

	virtual bool start();
	virtual void stop();

	virtual bool process(void* arg);

protected:
	unsigned long long sock;

};
