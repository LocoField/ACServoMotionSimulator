#pragma once

#include "ACServoMotionBase.h"

struct SMElement
{
	void* hMapFile;
	unsigned char* mapFileBuffer;
};

class ACServoMotionAssettoCorsa : public ACServoMotionBase
{
public:
	ACServoMotionAssettoCorsa() = default;
	virtual ~ACServoMotionAssettoCorsa() = default;

public:
	virtual char* getMotionName() override;

	virtual bool start();
	virtual void stop();

	virtual bool process(void* arg);

protected:
	SMElement graphics;
	SMElement physics;

	bool suspensionInitialized = false;
	float suspensionCenter[4] = { 0, };

};
