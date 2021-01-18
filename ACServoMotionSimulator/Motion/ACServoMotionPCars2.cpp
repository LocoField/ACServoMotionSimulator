#include "ACServoMotionPCars2.h"

#include <stdio.h>
#include <Windows.h>

#define SAFE_DELETE(p) if (p) { delete p; p = nullptr; }

const char* MAP_OBJECT_NAME = "$pcars2$";

char* ACServoMotionPCars2::getMotionName()
{
	return "Project Cars 2";
}

bool ACServoMotionPCars2::start()
{
	if (pDataLocal != nullptr)
		return true;

	hFileMapping = OpenFileMappingA(PAGE_READONLY, FALSE, MAP_OBJECT_NAME);
	if (hFileMapping == nullptr)
	{
		printf("Could not open file mapping object (%d).\n", GetLastError());

		return false;
	}

	// Get the data structure
	pDataMapped = (PCars2Data*)MapViewOfFile(hFileMapping, PAGE_READONLY, 0, 0, sizeof(PCars2Data));
	if (pDataMapped == nullptr)
	{
		printf("Could not map view of file (%d).\n", GetLastError());

		CloseHandle(hFileMapping);

		return false;
	}

	// Ensure we're sync'd to the correct data version
	if (pDataMapped->mVersion != SHARED_MEMORY_VERSION)
	{
		printf("Data version mismatch.\n");

		CloseHandle(hFileMapping);
		UnmapViewOfFile(pDataMapped);

		return false;
	}

	pDataLocal = new PCars2Data;
	return true;
}

void ACServoMotionPCars2::stop()
{
	if (hFileMapping != nullptr) CloseHandle(hFileMapping);
	if (pDataMapped != nullptr) UnmapViewOfFile(pDataMapped);
	if (pDataLocal != nullptr) SAFE_DELETE(pDataLocal);
}

bool ACServoMotionPCars2::process(void* arg)
{
	if (pDataMapped == nullptr)
		return false;

	while (pDataMapped->mSequenceNumber % 2 != 0)
	{
		// Odd sequence number indicates, that write into the shared memory is just happening
		Sleep(1);
	}

	memcpy(pDataLocal, pDataMapped, sizeof(PCars2Data));

	switch (pDataLocal->mGameState)
	{
		case GAME_INGAME_PAUSED:
		{
			return false;
		}
		case GAME_EXITED:
		case GAME_INGAME_RESTARTING:
		{
			angle_ = Vector3();
			axis_ = Vector4();

			suspensionInitialized = false;

			break;
		}
		case GAME_INGAME_PLAYING:
		{
			if (suspensionInitialized == false)
			{
				suspensionInitialized = true;

				for (int i = 0; i < TYRE_MAX; i++)
				{
					suspensionCenter[i] = pDataLocal->mSuspensionTravel[i];
				}
			}

			auto angleFilter = [](double& angle, double range)
			{
				angle = min(angle, range);
				angle = max(angle, -range);
			};

			// Euler angle
			double x = pDataLocal->mOrientation[0];
			double y = pDataLocal->mOrientation[1]; // heading
			double z = pDataLocal->mOrientation[2];

			double roll = (z * 180 / M_PI);
			double pitch = (x * 180 / M_PI);
			double yaw = (y * 180 / M_PI);

			angleFilter(roll, 15);
			angleFilter(pitch, 15);

			angle_.x = (float)roll;
			angle_.y = (float)pitch;

			// Suspension position
			axis_.ll = pDataLocal->mSuspensionTravel[0] - suspensionCenter[0];
			axis_.lr = pDataLocal->mSuspensionTravel[1] - suspensionCenter[1];
			axis_.rl = pDataLocal->mSuspensionTravel[2] - suspensionCenter[2];
			axis_.rr = pDataLocal->mSuspensionTravel[3] - suspensionCenter[3];

			break;
		}
	}

	return true;
}
