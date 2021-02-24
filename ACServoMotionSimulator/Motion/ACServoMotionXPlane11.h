#pragma once

#include "ACServoMotionBase.h"

#include <string>
#include <vector>

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

	bool initialize(const std::string& dataFilePath);

protected:
	std::vector<std::string> ACServoMotionXPlane11::parsingData(const std::string& data);

protected:
	std::string dataFilePath_;
	int numRecords_ = 0;

	std::streampos lastDataPosition = std::_BADOFF;
	float realTime_ = 0;

	int pitchIndex_ = -1;
	int rollIndex_ = -1;

};
