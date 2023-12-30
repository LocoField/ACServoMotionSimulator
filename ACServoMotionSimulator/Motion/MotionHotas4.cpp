#include "MotionHotas4.h"

#include <stdio.h>

#pragma pack(push, 1)
struct Hotas4Data
{
	unsigned char index;     // [1]
	unsigned short handleX;  // [0-1023]
	unsigned short handleY;  // [0-1023]
	unsigned char throttle;  // [ff-00]
	unsigned char twist;     // [00-ff]
	unsigned char pedals[3]; // [ff,ff,80]: left brake, right brake, rudder
	unsigned char rocker;    // [00-ff]
	unsigned char buttons1;  // [1|2|4|8|10|20|40|80]: hat-switch, R1, L1, R3, L3]
	unsigned char buttons2;  // [1|2|4|8|10|20|40|80]: ¡à, ¡¿, ¡Û, ¡â, R2, L2, SHARE, OPTIONS
};
#pragma pack(pop)

constexpr int angle = 15;

char* MotionHotas4::getMotionName()
{
	return "Hotas 4";
}

bool MotionHotas4::start()
{
	const USHORT idVendor = 1103;
	const USHORT idProduct = 46716;

	if (USBPcapHelper::findDevice(idVendor, idProduct) == false)
		return false;

	return USBPcapHelper::start();
}

void MotionHotas4::stop()
{
	USBPcapHelper::stop();
}

bool MotionHotas4::process(void* arg)
{
	float x = (float)handle[0];
	float y = (float)handle[1];
	x -= 0x1FF;
	y -= 0x1FF;

	motion_.roll = (x / 0x3FF) * angle;
	motion_.pitch = (y / 0x3FF) * angle;

	return true;
}

void MotionHotas4::processInterruptData(unsigned char* buffer, DWORD bytes)
{
	Hotas4Data data;
	memcpy(&data, buffer, sizeof(data));

	if (data.index != 1)
		return;

	handle[0] = data.handleX;
	handle[1] = data.handleY;
	handle[2] = data.twist;

	int currentLeftButtons = data.buttons2;
	if (lastButtonsLeft != currentLeftButtons)
	{
		if (callbackLeftButtonsEvent)
			callbackLeftButtonsEvent(currentLeftButtons);
	}

	int currentRightButtons = data.buttons1;
	if (lastButtonsRight != currentRightButtons)
	{
		if (callbackRightButtonsEvent)
			callbackRightButtonsEvent(currentRightButtons);
	}

	lastButtonsLeft = currentLeftButtons;
	lastButtonsRight = currentRightButtons;
}

void MotionHotas4::setLeftButtonsEventCallback(std::function<void(int)> callback)
{
	callbackLeftButtonsEvent = callback;
}

void MotionHotas4::setRightButtonsEventCallback(std::function<void(int)> callback)
{
	callbackRightButtonsEvent = callback;
}
