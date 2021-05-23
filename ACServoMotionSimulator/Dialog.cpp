#include "stdafx.h"
#include "Dialog.h"
#include "ACServoMotorHelper.h"

#include "Motion/ACServoMotionKeyboard.h"
#include "Motion/ACServoMotionPCars2.h"
#include "Motion/ACServoMotionNoLimits2.h"
#include "Motion/ACServoMotionXPlane11.h"

#include <thread>

#define DIALOG_TITLE "LocoField Motion Simulator"

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

void Dialog::initialize()
{
	motionTimer = new QTimer;
	connect(motionTimer, &QTimer::timeout, [this]()
	{
		Vector3 angleMotion;
		Vector4 axisMotion; // applied 4-axis system only

		if (motionSource)
		{
			if (motionSource->process(this) == false)
				return;

			motionSource->angle(angleMotion);
			motionSource->axis(axisMotion);

			angleMotion.x *= gain;
			angleMotion.y *= gain;
			angleMotion.z *= gain;
		}

		printf("%u\t%2.5f    %2.5f    %2.5f\n\t\t%2.5f    %2.5f    %2.5f    %2.5f\n",
			GetTickCount(), angleMotion.x, angleMotion.y, angleMotion.z,
			axisMotion.ll, axisMotion.lr, axisMotion.rl, axisMotion.rr);


		std::vector<int> targetPositions(numMotors, center);

		if (numMotors != 4)
			return;

		{
			int pz1, pz2, pz3, pz4;

			PlanePoints pp(width, height);
			pp.rotate(angleMotion.x, angleMotion.y);
			pp.getZPoints(pz1, pz2, pz3, pz4);

			// 625 (one rotation 2500 / 4 mm pitch)
			targetPositions[0] -= pz1 * 625;
			targetPositions[1] -= pz2 * 625;
			targetPositions[2] -= pz3 * 625;
			targetPositions[3] -= pz4 * 625;

			targetPositions[0] += axisMotion.ll * 1000 * 625;
			targetPositions[1] += axisMotion.lr * 1000 * 625;
			targetPositions[2] += axisMotion.rl * 1000 * 625;
			targetPositions[3] += axisMotion.rr * 1000 * 625;
		}

		for (int i = 0; i < numMotors; i++)
		{
			motionTriggers[i] = -1;

			int position = targetPositions[i] - currentPositions[i];

			if (abs(position) < angle)
				continue;

			if (currentPositions[i] + position < 0 ||
				currentPositions[i] + position >= limit)
				continue;

			int direction = position > 0 ? 1 : -1;
			int triggerIndex = direction > 0 ? 1 : 2;

			motionTriggers[i] = triggerIndex;
			currentPositions[i] += direction * angle;
		}

		for (int i = 0; i < numMotors; i++)
		{
			int trigger = motionTriggers[i];
			if (trigger > 0)
			{
				motor.trigger(i, motionTriggers[i]);
				motor.normal(i);
			}
		}

		updateUI(currentPositions);
		Sleep(1);
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

			motor.setDisconnectedCallback([buttonMotorConnect]()
			{
				buttonMotorConnect->setChecked(false);
			});

			connect(buttonMotorConnect, &QPushButton::toggled, [this, buttonMotorConnect](bool checked)
			{
				if (checked)
				{
					bool connect = motor.connect(portNames, baudRate, numMotors);

					if (connect == false)
					{
						buttonMotorConnect->setChecked(false);

						return;
					}

					for (int i = 0; i < numMotors; i++)
					{
						motor.setCycle(i, center * sign, 0);
						motor.setCycle(i, angle * sign, 1);
						motor.setCycle(i, -angle * sign, 2);

						motor.setSpeed(i, speed);
					}

					buttonMotorConnect->setText("Disconnect");
				}
				else
				{
					motor.disconnect();

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

					motor.power(true);
					motor.home();

					for (int i = 0; i < numMotors; i++)
						motor.trigger(i, 0);

					buttonMotorStart->setText("Stop");
				}
				else
				{
					motor.stop();
					Sleep(100);

					for (int i = 0; i < numMotors;)
					{
						int pos = 0;

						motor.position(i, pos);
						motor.setCycle(i, -pos, 3);
						motor.trigger(i, 3);

						i++;
					}

					motor.wait();

					for (int i = 0; i < numMotors; i++)
						currentPositions[i] = 0;

					updateUI(currentPositions);

					motor.power(false);

					buttonMotorStart->setText("2. Start");
				}
			});

			connect(buttonMotorEmergency, &QPushButton::clicked, [this](bool checked)
			{
				motor.emergency(checked);
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

						if (optionObject.find("angle") != optionObject.end())
						{
							angle = optionObject["angle"].toInt();
						}

						if (optionObject.find("gain") != optionObject.end())
						{
							gain = optionObject["gain"].toDouble();
						}

						if (optionObject.find("speed") != optionObject.end())
						{
							speed = optionObject["speed"].toInt();

							for (int i = 0; i < numMotors; i++)
							{
								motor.setSpeed(i, speed);
							}
						}
					}


					motionTriggers.assign(numMotors, -1);
					motionTimer->start();
				}
				else
				{
					motionTimer->stop();
					motionSource->stop();


					listMotionSource->setEnabled(true);
					buttonStart->setText("3. Start");

					QJsonObject optionObject = motionOptions["default"];
					angle = optionObject["angle"].toInt();
					gain = optionObject["gain"].toDouble();
					speed = optionObject["speed"].toInt();

					for (int i = 0; i < numMotors; i++)
					{
						motor.setSpeed(i, speed);
					}
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
		QAction* actionRepair = new QAction("Repair");
		actionRepair->setCheckable(true);
		connect(actionRepair, &QAction::triggered, [this, actionRepair](bool checked)
		{
			if (checked)
			{
				auto buttonMotorStart = findChild<QPushButton*>("buttonMotorStart");
				if (buttonMotorStart->isChecked())
				{
					actionRepair->setChecked(false);
					return;
				}

				for (int i = 0; i < numMotors; i++)
				{
					motor.setCycle(i, limit * sign, 3);
					motor.trigger(i, 3);
				}
			}
			else
			{
				motor.stop();
				Sleep(100);

				for (int i = 0; i < numMotors;)
				{
					int pos = 0;

					motor.position(i, pos);
					motor.setCycle(i, -pos, 3);
					motor.trigger(i, 3);

					i++;
				}
			}
		});

		menuAction->addAction(actionRepair);
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
					defaultOptionList.contains("gain") &&
					defaultOptionList.contains("limit") &&
					defaultOptionList.contains("numMotors") &&
					defaultOptionList.contains("port") &&
					defaultOptionList.contains("sign") &&
					defaultOptionList.contains("speed"))
				{
					angle = defaultOption["angle"].toInt();
					baudRate = defaultOption["baudRate"].toInt();
					center = defaultOption["center"].toInt();
					gain = defaultOption["gain"].toDouble();
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
