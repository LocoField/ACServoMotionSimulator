#include "stdafx.h"
#include "MotionController.h"
#include "ACServoMotorSerial.h"

#include "Motion/MotionBase.h"

class PlanePoints
{
	struct Point3i
	{
		int x;
		int y;
		int z;
	};

public:
	PlanePoints(int width, int height)
	{
		p1.x = -width / 2;
		p1.y = height / 2;
		p1.z = 0;

		p2.x = width / 2;
		p2.y = height / 2;
		p2.z = 0;

		p3.x = -width / 2;
		p3.y = -height / 2;
		p3.z = 0;

		p4.x = width / 2;
		p4.y = -height / 2;
		p4.z = 0;
	}

	void rotate(float roll, float pitch)
	{
		float a = -tan(roll * M_PI / 180);
		float b = tan(pitch * M_PI / 180);

		p1.z = -(a * p1.x + b * p1.y);
		p2.z = -(a * p2.x + b * p2.y);
		p3.z = -(a * p3.x + b * p3.y);
		p4.z = -(a * p4.x + b * p4.y);
	}

	void translate(float sway, float surge, float heave)
	{
		float sway_l = 0, sway_r = 0;
		float surge_f = 0, surge_b = 0;

		if (sway < 0)
			sway_l = -sway;
		else
			sway_r = sway;

		if (surge < 0)
			surge_f = -surge;
		else
			surge_b = surge;

		p1.z += heave + sway_l + surge_f;
		p2.z += heave + sway_r + surge_f;
		p3.z += heave + sway_l + surge_b;
		p4.z += heave + sway_r + surge_b;
	}

	void getZPoints(int& pz1, int& pz2, int& pz3, int& pz4)
	{
		pz1 = p1.z;
		pz2 = p2.z;
		pz3 = p3.z;
		pz4 = p4.z;
	}

protected:
	Point3i p1;
	Point3i p2;
	Point3i p3;
	Point3i p4;

};


void MotionController::setStepValue(int step)
{
	this->step = step;
}

void MotionController::setSpeedValue(int speed)
{
	this->speed = speed;
}

void MotionController::setMotionSize(int width, int height)
{
	this->width = width;
	this->height = height;
}

void MotionController::setCenterPosition(int center)
{
	this->center = center;
}

void MotionController::setLimitPosition(int limit)
{
	this->limit = limit;
}

void MotionController::setReverseDirection(bool reverse)
{
	this->sign = reverse ? -1 : 1;
}

bool MotionController::connect(const std::string& ports, int baudRate)
{
	size_t prev = 0, curr = 0;

	std::vector<std::string> portNames;

	while (curr != std::string::npos)
	{
		curr = ports.find(';', prev);
		std::string substring = ports.substr(prev, curr - prev);
		prev = curr + 1;

		portNames.push_back(substring);
	}


	bool succeed = true;

	for (size_t i = 0; i < portNames.size(); i++)
	{
		ACServoMotorSerial* motor = new ACServoMotorSerial;
		motors.emplace_back(motor);

		motor->setAddress((int)i + 1);
		succeed = motor->connect(portNames[i], baudRate);

		if (succeed == false)
		{
			disconnect();
			break;
		}
	}

	return succeed;
}

void MotionController::disconnect()
{
	for (auto& motor : motors)
	{
		motor->disconnect();
		delete motor;
	}

	motors.clear();
}

bool MotionController::power(bool on)
{
	for (auto& motor : motors)
	{
		motor->power(on);

		if (on)
		{
			motor->setSpeed(speed, 0);
			motor->setSpeed(speed, 1);
			motor->setSpeed(speed / 2, 2);
			motor->setSpeed(speed / 2, 3);

			motor->home();
		}
	}

	return true;
}

bool MotionController::start()
{
	if (motionTriggers.empty() == false)
		return false;

	motionTriggers.assign(motors.size(), -1);
	currentPositions.assign(motors.size(), center);

	power(true);
	Sleep(1000);

	for (auto& motor : motors)
	{
		motor->setCycle(center * sign, 0);

		motor->trigger(0);
		motor->normal();
	}

	for (auto& motor : motors)
	{
		bool retval = true;

		if (retval) retval = motor->setCycle(step, 0);
		if (retval) retval = motor->setCycle(-step, 1);
		if (retval) retval = motor->setCycle(step / 2, 2);
		if (retval) retval = motor->setCycle(-step / 2, 3);

		if (retval == false)
			return false;
	}

	return true;
}

bool MotionController::stop()
{
	motionTriggers.clear();
	currentPositions.clear();

	for (auto& motor : motors)
	{
		motor->setCycle(-center * sign, 0);

		motor->trigger(0);
		motor->normal();
	}

	Sleep(2000);
	power(false);

	return true;
}

void MotionController::executeMotion(const Motion& data)
{
	if (motionTriggers.empty())
		return;


	std::vector<int> targetPositions(motors.size(), center);
	motionTriggers.assign(motors.size(), -1);

	if (motors.size() == 4)
	{
		int pz1, pz2, pz3, pz4;

		PlanePoints pp(width, height);
		pp.rotate(data.roll, data.pitch);
		pp.translate(data.sway, data.surge, data.heave);
		pp.getZPoints(pz1, pz2, pz3, pz4);

		//printf("%d    %d    %d    %d\n\n", pz1, pz2, pz3, pz4);

		// 625 (one rotation 2500 / 4 mm pitch), but ...
		targetPositions[0] -= pz1 * 500;
		targetPositions[1] -= pz2 * 500;
		targetPositions[2] -= pz3 * 500;
		targetPositions[3] -= pz4 * 500;

		targetPositions[0] += data.ll * 250;
		targetPositions[1] += data.lr * 250;
		targetPositions[2] += data.rl * 250;
		targetPositions[3] += data.rr * 250;
	}


	std::thread t[4];

	for (size_t i = 0; i < motors.size(); i++)
		t[i] = std::thread(std::bind(&MotionController::motionThread, this, i));

	Sleep(1);

	for (size_t i = 0; i < motors.size(); i++)
	{
		if (targetPositions[i] < 0 || targetPositions[i] > limit)
			continue;

		int position = targetPositions[i] - currentPositions[i];
		int direction = position > 0 ? 1 : -1;
		int triggerIndex = direction > 0 ? 0 : 1;

		if (abs(position) < step / 2)
			continue;

		if (abs(position) < step)
		{
			step /= 2;
			triggerIndex += 2;
		}

		motionTriggers[i] = triggerIndex;
		currentPositions[i] += (step * direction);

		if (currentPositions[i] < 0 ||
			currentPositions[i] >= limit)
		{
			motionTriggers[i] = -1;
			currentPositions[i] -= (step * direction);
		}
	}

	motionWaiter.notify_all();

	for (size_t i = 0; i < motors.size(); i++)
		t[i].join();
}

void MotionController::motionThread(size_t index)
{
	std::shared_lock<std::shared_mutex> locker(motionMutex);

	motionWaiter.wait(locker);

	int trigger = motionTriggers[index];
	if (trigger >= 0)
	{
		motors[index]->trigger(motionTriggers[index]);
		motors[index]->normal();
	}
}
