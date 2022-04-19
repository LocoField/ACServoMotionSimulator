#include "ACServoMotionAssettoCorsa.h"
#include "AssettoCorsaData.h"

#include <stdio.h>
#include <Windows.h>

const char* MAP_OBJECT_NAME1 = "Local\\acpmf_physics";
const char* MAP_OBJECT_NAME2 = "Local\\acpmf_graphics";

char* ACServoMotionAssettoCorsa::getMotionName()
{
	return "Project Cars 2";
}

bool ACServoMotionAssettoCorsa::start()
{
	bool retval = true;

	physics.hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SPageFilePhysics), MAP_OBJECT_NAME1);
	if (physics.hMapFile == nullptr)
	{
		retval = false;
		printf("Could not create file mapping object (%d).\n", GetLastError());
	}

	physics.mapFileBuffer = (unsigned char*)MapViewOfFile(physics.hMapFile, FILE_MAP_READ, 0, 0, sizeof(SPageFilePhysics));
	if (physics.hMapFile == nullptr)
	{
		retval = false;
		printf("Could not map view of file (%d).\n", GetLastError());
	}


	graphics.hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SPageFileGraphic), MAP_OBJECT_NAME2);
	if (graphics.hMapFile == nullptr)
	{
		retval = false;
		printf("Could not create file mapping object (%d).\n", GetLastError());
	}

	graphics.mapFileBuffer = (unsigned char*)MapViewOfFile(graphics.hMapFile, FILE_MAP_READ, 0, 0, sizeof(SPageFileGraphic));
	if (graphics.hMapFile == nullptr)
	{
		retval = false;
		printf("Could not map view of file (%d).\n", GetLastError());
	}


	if (retval == false)
	{
		stop();
	}

	return true;
}

void ACServoMotionAssettoCorsa::stop()
{
	if (physics.mapFileBuffer)
		UnmapViewOfFile(physics.mapFileBuffer);

	if (physics.hMapFile)
		CloseHandle(physics.hMapFile);


	if (graphics.mapFileBuffer)
		UnmapViewOfFile(graphics.mapFileBuffer);

	if (graphics.hMapFile)
		CloseHandle(graphics.hMapFile);
}

bool ACServoMotionAssettoCorsa::process(void* arg)
{
	if (physics.mapFileBuffer == nullptr || graphics.mapFileBuffer == nullptr)
		return false;

	SPageFilePhysics* pf = (SPageFilePhysics*)physics.mapFileBuffer;
	SPageFileGraphic* pg = (SPageFileGraphic*)graphics.mapFileBuffer;

	switch (pg->status)
	{
		case AC_PAUSE:
		{
			return false;
		}
		case AC_OFF:
		{
			memset(&motion_, 0, sizeof(motion_));

			suspensionInitialized = false;

			break;
		}
		case AC_LIVE:
		case AC_REPLAY:
		{
			if (suspensionInitialized == false)
			{
				suspensionInitialized = true;

				for (int i = 0; i < 4; i++)
				{
					suspensionCenter[i] = pf->suspensionTravel[i];
				}

				return false;
			}

			auto angleFilter = [](float& angle, float range)
			{
				angle = min(angle, range);
				angle = max(angle, -range);
			};

			motion_.roll = pf->roll;
			motion_.pitch = pf->pitch;

			// Suspension position
			motion_.ll = (pf->suspensionTravel[0] - suspensionCenter[0]) * 1000;
			motion_.lr = (pf->suspensionTravel[1] - suspensionCenter[1]) * 1000;
			motion_.rl = (pf->suspensionTravel[2] - suspensionCenter[2]) * 1000;
			motion_.rr = (pf->suspensionTravel[3] - suspensionCenter[3]) * 1000;

			angleFilter(motion_.roll, 15);
			angleFilter(motion_.pitch, 15);
			angleFilter(motion_.ll, 15);
			angleFilter(motion_.lr, 15);
			angleFilter(motion_.rl, 15);
			angleFilter(motion_.rr, 15);

			break;
		}
	}

	return true;
}
