#pragma once

#include "MotionBase.h"

#include <USBPcapHelper.h>

#include <functional>
#include <vector>

class MotionHotas4 : public MotionBase, public USBPcapHelper
{
public:
	/* for buttons2 */
	enum LeftButtons
	{
		BUTTON_NONE = 0,
		BUTTON_RECTANGLE = 1,
		BUTTON_CROSS = 2,
		BUTTON_CIRCLE = 4,
		BUTTON_TRIANGLE = 8,

		BUTTON_R2 = 10,
		BUTTON_L2 = 20,
		BUTTON_SHARE = 40,
		BUTTON_OPTIONS = 80,
	};

	/* for buttons1 */
	enum RightButtons
	{
		SWITCH_HAT_UP = 0,
		SWITCH_HAT_UP_RIGHT = 1,
		SWITCH_HAT_RIGHT = 2,
		SWITCH_HAT_DOWN_RIGHT = 3,
		SWITCH_HAT_DOWN = 4,
		SWITCH_HAT_DOWN_LEFT = 5,
		SWITCH_HAT_LEFT = 6,
		SWITCH_HAT_UP_LEFT = 7,
		SWITCH_HAT_CENTER = 8,

		BUTTON_R1 = 10,
		BUTTON_L1 = 20,
		BUTTON_R3 = 40,
		BUTTON_L3 = 80,
	};

public:
	MotionHotas4() = default;
	virtual ~MotionHotas4() = default;

public:
	virtual char* getMotionName() override;
	
	virtual bool start() override;
	virtual void stop() override;

	virtual bool process(void* arg);

	virtual void processInterruptData(unsigned char* buffer, DWORD bytes) override;

public:
	void setLeftButtonsEventCallback(std::function<void(int)> callback);
	void setRightButtonsEventCallback(std::function<void(int)> callback);

private:
	std::vector<unsigned char*> data;

	int handle[3] = { 0x1FF, 0x1FF, 0x7F };
	int lastButtonsLeft = (int)LeftButtons::BUTTON_NONE;
	int lastButtonsRight = (int)RightButtons::SWITCH_HAT_CENTER;

	std::function<void(int)> callbackLeftButtonsEvent = nullptr;
	std::function<void(int)> callbackRightButtonsEvent = nullptr;

};
