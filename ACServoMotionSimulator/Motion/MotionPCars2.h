#pragma once

#include "MotionBase.h"
#include "PCars2Data.h"

class MotionPCars2 : public MotionBase
{
public:
	MotionPCars2() = default;
	virtual ~MotionPCars2() = default;

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
