#pragma once

#include "ACServoMotionBase.h"
#include "PCars2Data.h"

class ACServoMotionPCars2 : public ACServoMotionBase
{
public:
	ACServoMotionPCars2() = default;
	virtual ~ACServoMotionPCars2() = default;

public:
	virtual char* getMotionName() override;

	virtual bool start();
	virtual void stop();

	virtual bool process(void* arg);

protected:
	void* hFileMapping = nullptr;
	const PCars2Data* pDataMapped = nullptr;
	PCars2Data* pDataLocal = nullptr;

	bool suspensionInitialized = false;
	float suspensionCenter[4] = { 0, };

};
