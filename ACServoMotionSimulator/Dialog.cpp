#include "stdafx.h"
#include "Dialog.h"
#include "ACServoMotorHelper.h"

#include "Motion/ACServoMotionKeyboard.h"
#include "Motion/ACServoMotionPCars2.h"
#include "Motion/ACServoMotionNoLimits2.h"

#define DIALOG_TITLE "LocoField Motion Simulator"

Dialog::Dialog()
{
	bool retval = loadOption();

	initialize();

	addMotionModules();

	if (retval)
		updateUI(currentPositions);
	else
		setDisabled(true);
}

Dialog::~Dialog()
{
}

void Dialog::initialize()
{
	motionTimer = new QTimer;
	connect(motionTimer, &QTimer::timeout, [this]()
	{
		Sleep(1);

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

		if (numMotors == 2)
		{
			targetPositions[0] += (angleMotion.x * angle);
			targetPositions[1] -= (angleMotion.x * angle);
			targetPositions[0] -= (angleMotion.y * angle);
			targetPositions[1] -= (angleMotion.y * angle);
		}
		else if (numMotors == 4)
		{
			targetPositions[0] += (angleMotion.x * angle);
			targetPositions[1] -= (angleMotion.x * angle);
			targetPositions[2] += (angleMotion.x * angle);
			targetPositions[3] -= (angleMotion.x * angle);

			targetPositions[0] += (angleMotion.y * angle);
			targetPositions[1] += (angleMotion.y * angle);
			targetPositions[2] -= (angleMotion.y * angle);
			targetPositions[3] -= (angleMotion.y * angle);

			targetPositions[0] += axisMotion.ll * 1000 * 2000; // meter * motor rotation by pitch (10000 / 5)
			targetPositions[1] += axisMotion.lr * 1000 * 2000;
			targetPositions[2] += axisMotion.rl * 1000 * 2000;
			targetPositions[3] += axisMotion.rr * 1000 * 2000;
		}

		for (int i = 0; i < numMotors; i++)
		{
			int position = targetPositions[i] - currentPositions[i];

			if (abs(position) < angle)
				continue;

			if (currentPositions[i] + position < 0 ||
				currentPositions[i] + position >= limit)
				continue;

			int direction = position > 0 ? 1 : -1;
			int triggerIndex = direction > 0 ? 1 : 2;

			motor.trigger(triggerIndex, i);

			currentPositions[i] += direction * angle;
		}

		updateUI(currentPositions);
	});

	{
		auto groupBox = new QGroupBox("Motor");

		motorLayout = new QVBoxLayout;
		motorLayout->addWidget(groupBox);

		{
			auto buttonMotorConnect = new QPushButton("1. Connect");
			buttonMotorConnect->setFocusPolicy(Qt::FocusPolicy::NoFocus);
			buttonMotorConnect->setCheckable(true);
			buttonMotorConnect->setFixedWidth(150);
			buttonMotorConnect->setFixedHeight(100);

			auto buttonMotorStart = new QPushButton("2. Start");
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
					bool connect = motor.connect(portName, numMotors);

					if (connect == false)
					{
						buttonMotorConnect->setChecked(false);

						return;
					}

					motor.setPosition(angle * sign, 1, -1, false);
					motor.setPosition(-angle * sign, 2, -1, false);

					motor.setSpeed(speed, 0);

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
					{
						if (motor.setPosition(center * sign, 0, i) == false)
							continue;

						currentPositions[i] = center;
					}

					buttonMotorStart->setText("Stop");
				}
				else
				{
					// why index required to stop motors?
					motor.stop(0);
					motor.stop(1);
					motor.stop(2);

					for (int i = 0; i < numMotors;)
					{
						int position = 0;
						bool moving = true;

						motor.position(i, position, moving);
						motor.setPosition(-position, 0, i);

						i++;
					}

					buttonMotorStart->setText("2. Start");
				}
			});

			connect(buttonMotorEmergency, &QPushButton::clicked, [this](bool checked)
			{
				for (int i = 0; i < numMotors; i++)
				{
					auto command = ACServoMotorHelper::emergency(checked, i + 1);
					motor.write({ (char*)command.data(), (int)command.size() });

					Sleep(10);
				}

				motor.read();
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

	{
		auto groupBox = new QGroupBox("Controller");

		controllerLayout = new QVBoxLayout;
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

							motor.setSpeed(speed, 1);
							motor.setSpeed(speed, 2);
						}
					}

					listMotionSource->setEnabled(false);
					motionTimer->start();
				}
				else
				{
					buttonStart->setText("3. Start");

					motionTimer->stop();
					motionSource->stop();

					QJsonObject optionObject = motionOptions["default"];
					angle = optionObject["angle"].toInt();
					gain = optionObject["gain"].toDouble();
					speed = optionObject["speed"].toInt();

					motor.setSpeed(speed, 1);
					motor.setSpeed(speed, 2);

					listMotionSource->setEnabled(true);
				}
			});

			auto layout = new QHBoxLayout;
			layout->setAlignment(Qt::AlignLeft);
			layout->addWidget(listMotionSource);
			layout->addWidget(buttonStart);

			groupBox->setLayout(layout);
		}
	}

	mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(motorLayout);
	mainLayout->addLayout(controllerLayout);

	setMinimumWidth(600);

	setWindowTitle(DIALOG_TITLE);
	setWindowFlag(Qt::WindowMinimizeButtonHint);


	QMenuBar* menu = new QMenuBar();
	layout()->setMenuBar(menu);

	QMenu* menuView = new QMenu("Option");
	{
		QAction* actionLoadOption = new QAction("Load");
		connect(actionLoadOption, &QAction::triggered, [&]()
		{
			if (loadOption() == false)
				setDisabled(true);
		});

		menuView->addAction(actionLoadOption);
		menu->addMenu(menuView);
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
					defaultOptionList.contains("center") &&
					defaultOptionList.contains("gain") &&
					defaultOptionList.contains("limit") &&
					defaultOptionList.contains("numMotors") &&
					defaultOptionList.contains("port") &&
					defaultOptionList.contains("sign") &&
					defaultOptionList.contains("speed"))
				{
					angle = defaultOption["angle"].toInt();
					center = defaultOption["center"].toInt();
					gain = defaultOption["gain"].toDouble();
					limit = defaultOption["limit"].toInt();
					numMotors = defaultOption["numMotors"].toInt();
					portName = defaultOption["port"].toString();
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
	installEventFilter(motionKeyboard); // for receiving QKeyEvent
	addModule(motionKeyboard);

	ACServoMotionPCars2* motionPCars2 = new ACServoMotionPCars2;
	addModule(motionPCars2);

	ACServoMotionNoLimits2* motionNoLimits2 = new ACServoMotionNoLimits2;
	addModule(motionNoLimits2);

	// dynamic load
	

	listMotionSource->setCurrentRow(0);
}
