#include "stdafx.h"
#include "Dialog.h"
#include "ACServoMotorHelper.h"

#include "Motion/ACServoMotionKeyboard.h"
#include "Motion/ACServoMotionPCars2.h"
#include "Motion/ACServoMotionNoLimits2.h"
#include "Motion/ACServoMotionXPlane11.h"

#include <thread>

#define DIALOG_TITLE "LocoField Motion Simulator v2.1"

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

	void translate(float heave, float sway, float surge)
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

Dialog::Dialog()
{
	bool retval = loadOption();

	initialize();

	addMotionModules();

	if (retval)
		updateUI(currentPositions);
	else
		mainWidget->setDisabled(true);
}

Dialog::~Dialog()
{
	clearMotionModules();
}

void Dialog::motionThread(int index)
{
	std::shared_lock<std::shared_mutex> locker(motionMutex);

	motionWaiter.wait(locker);

	int trigger = motionTriggers[index];
	if (trigger >= 0)
	{
		motors[index].trigger(motionTriggers[index]);
		motors[index].normal();
	}
}

void Dialog::initialize()
{
	motionTimer = new QTimer;
	connect(motionTimer, &QTimer::timeout, [this]()
	{
		Motion data;

		if (motionSource)
		{
			if (motionSource->process(this) == false)
				return;

			motionSource->motion(data);
		}

#ifdef _DEBUG
		printf("%2.5f    %2.5f    %2.5f\n%2.5f    %2.5f    %2.5f\n%2.5f    %2.5f    %2.5f    %2.5f\n%u\n",
			data.roll, data.pitch, data.heave,
			data.yaw, data.sway, data.surge,
			data.ll, data.lr, data.rl, data.rr,
			GetTickCount());
#endif


		std::vector<int> targetPositions(numMotors, center);
		motionTriggers.assign(numMotors, -1);

		if (numMotors == 4)
		{
			int pz1, pz2, pz3, pz4;

			PlanePoints pp(width, height);
			pp.rotate(data.roll, data.pitch);
			pp.translate(data.heave, data.sway, data.surge);
			pp.getZPoints(pz1, pz2, pz3, pz4);

			//printf("%d    %d    %d    %d\n\n", pz1, pz2, pz3, pz4);

			// 625 (one rotation 2500 / 4 mm pitch), but ...
			targetPositions[0] -= pz1 * 500;
			targetPositions[1] -= pz2 * 500;
			targetPositions[2] -= pz3 * 500;
			targetPositions[3] -= pz4 * 500;

			targetPositions[0] += data.ll * 500;
			targetPositions[1] += data.lr * 500;
			targetPositions[2] += data.rl * 500;
			targetPositions[3] += data.rr * 500;
		}


		std::thread t[4];

		for (int i = 0; i < numMotors; i++)
			t[i] = std::thread(std::bind(&Dialog::motionThread, this, i));

		Sleep(1);

		for (int i = 0; i < numMotors; i++)
		{
			if (targetPositions[i] < 0 || targetPositions[i] > limit)
				continue;

			int step = angle;
			int position = targetPositions[i] - currentPositions[i];
			int direction = position > 0 ? 1 : -1;
			int triggerIndex = direction > 0 ? 0 : 1;

			if (abs(position) < step)
			{
				continue;
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

		for (int i = 0; i < numMotors; i++)
			t[i].join();

		updateUI(currentPositions);
	});

	auto motorLayout = new QVBoxLayout;
	{
		auto groupBox = new QGroupBox("Motor");
		motorLayout->addWidget(groupBox);

		{
			auto buttonMotorConnect = new QPushButton("1. Connect");
			buttonMotorConnect->setFocusPolicy(Qt::FocusPolicy::NoFocus);
			buttonMotorConnect->setCheckable(true);
			buttonMotorConnect->setFixedWidth(150);
			buttonMotorConnect->setFixedHeight(100);

			auto buttonMotorStart = new QPushButton("2. Start");
			buttonMotorStart->setObjectName("buttonMotorStart");
			buttonMotorStart->setFocusPolicy(Qt::FocusPolicy::NoFocus);
			buttonMotorStart->setCheckable(true);
			buttonMotorStart->setFixedWidth(150);
			buttonMotorStart->setFixedHeight(100);

			auto buttonMotorEmergency = new QPushButton("EMERGENCY");
			buttonMotorEmergency->setFocusPolicy(Qt::FocusPolicy::NoFocus);
			buttonMotorEmergency->setCheckable(true);
			buttonMotorEmergency->setFixedWidth(150);
			buttonMotorEmergency->setFixedHeight(100);
			buttonMotorEmergency->setShortcut(Qt::Key_Space);

			auto layoutLabels = new QVBoxLayout;
			{
				for (int i = 0; i < numMotors; i++)
				{
					auto labelMotorPosition = new QLabel(QString("Motor %1\t: ").arg(i + 1));

					layoutLabels->addWidget(labelMotorPosition);
				}
			}

			auto layoutValues = new QVBoxLayout;
			{
				for (int i = 0; i < numMotors; i++)
				{
					auto valueMotorPosition = new QLabel("0");
					valueMotorPosition->setObjectName(QString("valueMotorPosition%1").arg(i));
					valueMotorPosition->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
					valueMotorPosition->setFixedWidth(100);

					layoutValues->addWidget(valueMotorPosition);
				}
			}

			connect(buttonMotorConnect, &QPushButton::toggled, [this, buttonMotorConnect](bool checked)
			{
				if (checked)
				{
					bool succeed = true;
					auto ports = portNames.split(';');

					for (int i = 0; i < numMotors; i++)
					{
						motors[i].setAddress(i + 1);
						succeed = motors[i].connect(ports[i].toStdString(), baudRate);

						if (succeed == false)
						{
							buttonMotorConnect->setChecked(false);
							return;
						}
					}

					for (int i = 0; i < numMotors; i++)
						motors[i].setSpeed(speed);

					buttonMotorConnect->setText("Disconnect");
				}
				else
				{
					for (int i = 0; i < numMotors; i++)
						motors[i].disconnect();

					buttonMotorConnect->setText("1. Connect");
				}
			});

			connect(buttonMotorStart, &QPushButton::clicked, [this, buttonMotorStart](bool checked)
			{
				if (checked)
				{
					for (int i = 0; i < numMotors; i++)
						currentPositions[i] = center;

					updateUI(currentPositions);

					for (int i = 0; i < numMotors; i++)
					{
						motors[i].power(true);
						motors[i].home();
					}

					Sleep(1000);

					for (int i = 0; i < numMotors; i++)
					{
						motors[i].setCycle(center * sign, 0);

						motors[i].trigger(0);
						motors[i].normal();
					}

					for (int i = 0; i < numMotors; i++)
					{
						motors[i].setCycle(angle, 0);
						motors[i].setCycle(-angle, 1);
					}

					buttonMotorStart->setText("Stop");
				}
				else
				{
					for (int i = 0; i < numMotors; i++)
					{
						int pos = 0;
						bool moving = true;

						motors[i].position(pos, moving);

						motors[i].setCycle(-pos, 0);

						motors[i].trigger(0);
						motors[i].normal();
					}

					for (int i = 0; i < numMotors; i++)
					{
						motors[i].power(false);

						currentPositions[i] = 0;
					}

					updateUI(currentPositions);

					buttonMotorStart->setText("2. Start");
				}
			});

			connect(buttonMotorEmergency, &QPushButton::clicked, [this](bool checked)
			{
				for (int i = 0; i < numMotors; i++)
					motors[i].emergency(checked);
			});

			auto layout = new QHBoxLayout;
			layout->setAlignment(Qt::AlignLeft);
			layout->addWidget(buttonMotorConnect);
			layout->addWidget(buttonMotorStart);
			layout->addWidget(buttonMotorEmergency);
			layout->addLayout(layoutLabels);
			layout->addLayout(layoutValues);

			groupBox->setLayout(layout);
		}
	}

	auto controllerLayout = new QVBoxLayout;
	{
		auto groupBox = new QGroupBox("Controller");
		controllerLayout->addWidget(groupBox);

		{
			auto listMotionSource = new QListWidget;
			listMotionSource->setObjectName("listMotionSource");
			listMotionSource->setFixedWidth(200);
			listMotionSource->setFixedHeight(100);

			auto buttonStart = new QPushButton("3. Start");
			buttonStart->setFocusPolicy(Qt::FocusPolicy::NoFocus);
			buttonStart->setCheckable(true);
			buttonStart->setFixedWidth(100);
			buttonStart->setFixedHeight(100);

			connect(buttonStart, &QPushButton::toggled, [this, listMotionSource, buttonStart](bool checked)
			{
				if (checked)
				{
					listMotionSource->setEnabled(false);
					buttonStart->setText("Stop");

					motionSource = motionSources[listMotionSource->currentRow()];

					if (motionSource->start() == false)
					{
						buttonStart->setChecked(false);
						return;
					}

					QString motionName = listMotionSource->currentItem()->text();
					if (motionOptions.find(motionName) != motionOptions.end())
					{
						QJsonObject optionObject = motionOptions[motionName];

						// change options for motions
					}

					motionTimer->start();
				}
				else
				{
					motionTimer->stop();
					motionSource->stop();


					listMotionSource->setEnabled(true);
					buttonStart->setText("3. Start");

					QJsonObject optionObject = motionOptions["default"];

					// recover options
				}
			});

			auto layout = new QHBoxLayout;
			layout->setAlignment(Qt::AlignLeft);
			layout->addWidget(listMotionSource);
			layout->addWidget(buttonStart);

			groupBox->setLayout(layout);
		}
	}

	auto mainLayout = new QVBoxLayout;
	mainLayout->addLayout(motorLayout);
	mainLayout->addLayout(controllerLayout);

	mainWidget = new QWidget;
	mainWidget->setLayout(mainLayout);

	mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(mainWidget);


	setMinimumWidth(600);

	setWindowTitle(DIALOG_TITLE);
	setWindowFlag(Qt::WindowMinimizeButtonHint);


	QMenuBar* menu = new QMenuBar();
	layout()->setMenuBar(menu);

	QMenu* menuAction = new QMenu("Action");
	{
		QAction* actionPowerOff = new QAction("Power Off");

		connect(actionPowerOff, &QAction::triggered, [this]()
		{
			for (int i = 0; i < numMotors; i++)
				motors[i].power(false);
		});

		menuAction->addAction(actionPowerOff);
		menu->addMenu(menuAction);
	}

	QMenu* menuOption = new QMenu("Option");
	{
		QAction* actionLoad = new QAction("Load");
		connect(actionLoad, &QAction::triggered, [&]()
		{
			if (loadOption())
			{
				clearMotionModules();
				addMotionModules();

				mainWidget->setEnabled(true);
			}
			else
			{
				mainWidget->setDisabled(true);
			}
		});

		menuOption->addAction(actionLoad);
		menu->addMenu(menuOption);
	}
}

void Dialog::updateUI(const std::vector<int>& positions)
{
	for (int i = 0; i < numMotors; i++)
	{
		auto valueMotorPosition = findChild<QLabel*>(QString("valueMotorPosition%1").arg(i));
		valueMotorPosition->setText(QString::number(positions[i]));
	}
}

bool Dialog::loadOption()
{
	bool retval = true;

	QString filepath = QCoreApplication::applicationDirPath();
	QFile loadFile(filepath + "/option.txt");

	if (loadFile.open(QIODevice::ReadOnly) == false)
	{
		retval = false;
	}
	else
	{
		QJsonDocument doc = QJsonDocument::fromJson(loadFile.readAll());

		if (doc.isNull())
		{
			retval = false;
		}
		else
		{
			QJsonObject optionObject = doc.object();
			auto optionList = optionObject.keys();

			if (optionList.contains("default") == false)
			{
				retval = false;
			}
			else
			{
				QJsonObject defaultOption = optionObject["default"].toObject();
				auto defaultOptionList = defaultOption.keys();

				if (defaultOptionList.contains("angle") &&
					defaultOptionList.contains("baudRate") &&
					defaultOptionList.contains("center") &&
					defaultOptionList.contains("limit") &&
					defaultOptionList.contains("numMotors") &&
					defaultOptionList.contains("port") &&
					defaultOptionList.contains("sign") &&
					defaultOptionList.contains("speed"))
				{
					angle = defaultOption["angle"].toInt();
					baudRate = defaultOption["baudRate"].toInt();
					center = defaultOption["center"].toInt();
					limit = defaultOption["limit"].toInt();
					numMotors = defaultOption["numMotors"].toInt();
					portNames = defaultOption["port"].toString();
					sign = defaultOption["sign"].toInt();
					speed = defaultOption["speed"].toInt();
				}
				else
				{
					retval = false;
				}

				motionOptions.clear();

				for (auto it = optionList.cbegin(); it != optionList.cend(); ++it)
				{
					motionOptions.insert({ *it, optionObject[*it].toObject() });
				}
			}
		}

		loadFile.close();
	}

	if (retval == false)
	{
		printf("ERROR: loadOption() failed.\n");
		return false;
	}

	motors.clear();
	motors.resize(numMotors);
	currentPositions.resize(numMotors);

	return true;
}

void Dialog::addMotionModules()
{
	auto listMotionSource = findChild<QListWidget*>("listMotionSource");

	auto addModule = [this, listMotionSource](ACServoMotionBase* motion)
	{
		listMotionSource->addItem(motion->getMotionName());

		motionSources.emplace_back(motion);
	};

	// default module
	ACServoMotionKeyboard* motionKeyboard = new ACServoMotionKeyboard;
	{
		installEventFilter(motionKeyboard); // for receiving QKeyEvent

		addModule(motionKeyboard);
	}

	ACServoMotionPCars2* motionPCars2 = new ACServoMotionPCars2;
	{
		addModule(motionPCars2);
	}

	ACServoMotionNoLimits2* motionNoLimits2 = new ACServoMotionNoLimits2;
	{
		addModule(motionNoLimits2);
	}

	ACServoMotionXPlane11* motionXPlane11 = new ACServoMotionXPlane11;
	{
		addModule(motionXPlane11);
	}

	// dynamic load
	

	listMotionSource->setCurrentRow(0);
}

void Dialog::clearMotionModules()
{
	auto listMotionSource = findChild<QListWidget*>("listMotionSource");
	listMotionSource->clear();

	for (auto motion : motionSources)
	{
		delete motion;
	}

	motionSources.clear();
}
