#pragma once

#include "MotionBase.h"

#pragma pack(push, 1)
struct Message
{
	unsigned short type_id;
	unsigned int request_id;
	char data[1];
};

struct TelemetryData
{
	int state;
	int rendered_frame;
	int view_mode;
	int current_coaster;
	int current_style_id;
	int current_train;
	int current_car;
	int current_seat;
	float speed;
	float position_x;
	float position_y;
	float position_z;
	float rotation_x;
	float rotation_y;
	float rotation_z;
	float rotation_w;
	float gforce_x;
	float gforce_y;
	float gforce_z;
};
#pragma pack(pop)

class MotionNoLimits2 : public MotionBase
{
public:
	MotionNoLimits2();
	virtual ~MotionNoLimits2() = default;

public:
	virtual char* getMotionName() override;

	virtual bool start();
	virtual void stop();

	virtual bool process(void* arg);

protected:
	unsigned long long sock;
	int last_rendered_frame;

};
