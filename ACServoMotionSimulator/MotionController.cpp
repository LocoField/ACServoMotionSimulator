#include "stdafx.h"
#include "MotionController.h"

#include <QtSerialPort/QSerialPort>

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

MotionController::MotionController()
{
	board = new QSerialPort;
}

MotionController::~MotionController()
{
	delete board;
	board = nullptr;
}

void MotionController::setMotionSize(int width, int height)
{
	this->width = width;
	this->height = height;
}

void MotionController::setCenterPosition(int center)
{
	
}

void MotionController::setLimitPosition(int limit)
{
	
}

bool MotionController::connect(const std::string& portName, int baudRate)
{
	board->setPortName(QString::fromStdString(portName));
	board->setBaudRate(baudRate);

	return board->open(QIODevice::ReadWrite);
}

void MotionController::disconnect()
{
	board->close();
}

bool MotionController::power(bool on)
{
	board->write(QString("power %1\n").arg(on).toLocal8Bit());
	auto d = board->readAll();

#ifdef _DEBUG
	printf("%s\n", d.constData());
#endif

	return true;
}

bool MotionController::start()
{
	power(true);
	Sleep(2000);

	lastPositions.resize(4);

	board->write(QString("ready %1\n").arg(true).toLocal8Bit());
	auto d = board->readAll();

#ifdef _DEBUG
	printf("%s\n", d.constData());
#endif

	return true;
}

bool MotionController::stop()
{
	board->write(QString("ready %1\n").arg(false).toLocal8Bit());
	auto d = board->readAll();

#ifdef _DEBUG
	printf("%s\n", d.constData());
#endif

	Sleep(2000);
	power(false);

	return true;
}

void MotionController::motion(const Motion& data)
{
	std::vector<int> position(4);

	int pz1, pz2, pz3, pz4;

	PlanePoints pp(width, height);
	pp.rotate(data.roll, data.pitch);
	pp.translate(data.sway, data.surge, data.heave);
	pp.getZPoints(pz1, pz2, pz3, pz4);

	// 625 (one rotation 2500 / 4 mm pitch), but ...
	position[0] -= pz1 * angleStep;
	position[1] -= pz2 * angleStep;
	position[2] -= pz3 * angleStep;
	position[3] -= pz4 * angleStep;

	position[0] += data.ll * linearStep;
	position[1] += data.lr * linearStep;
	position[2] += data.rl * linearStep;
	position[3] += data.rr * linearStep;

	if (lastPositions == position)
		return;

	board->write(QString("position %1 %2 %3 %4\n").arg(position[0]).arg(position[1]).arg(position[2]).arg(position[3]).toLocal8Bit());
	auto d = board->readAll();

#ifdef _DEBUG
	printf("%d    %d    %d    %d\n%u\n", position[0], position[1], position[2], position[3], GetTickCount());
	printf("%s\n", d.constData());
#endif

	lastPositions = position;
}
