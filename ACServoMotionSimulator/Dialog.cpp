#include "stdafx.h"
#include "Dialog.h"
#include "ACServoMotorHelper.h"

#include "Motion/ACServoMotionKeyboard.h"
#include "Motion/ACServoMotionPCars2.h"

#define DIALOG_TITLE "LocoField Motion Simulator"

Dialog::Dialog()
{
	loadOption();

	initialize();

	addMotionModules();
}

Dialog::~Dialog()
{
}

void Dialog::initialize()
{
	motionTimer = new QTimer;
	connect(motionTimer, &QTimer::timeout, [this]()
	{
		Vector3 angleMotion;

		if (motionSource)
		{
			if (motionSource->process(this) == false)
				return;

			motionSource->position(angleMotion);
		}

		printf("%f    %f    %f\n", angleMotion.x, angleMotion.y, angleMotion.z);


		if (motor.isConnected() == false)
			return;

		std::vector<int> motorPositions(numMotors);

		for (int i = 0; i < numMotors; i++)
		{
			bool moving = true;

			motor.position(i, motorPositions[i], moving);
			motorPositions[i] *= sign;
		}

		updateUI(motorPositions);


		std::vector<int> targetPositions = centerPositions;

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
		}

		for (int i = 0; i < numMotors; i++)
		{
			int position = targetPositions[i] - motorPositions[i];

			if (abs(position) < angle)
				continue;

			if (motorPositions[i] + position < 0 ||
				motorPositions[i] + position >= limitPositions[i])
				continue;

			int triggerIndex = position > 0 ? 1 : 2;

			motor.trigger(triggerIndex, i);
		}
	});

	{
		auto groupBox = new QGroupBox("Motor");

		motorLayout = new QVBoxLayout;
		motorLayout->addWidget(groupBox);

		{
			auto buttonConnect = new QPushButton("Connect");
			buttonConnect->setFocusPolicy(Qt::FocusPolicy::NoFocus);
			buttonConnect->setCheckable(true);
			buttonConnect->setFixedWidth(150);
			buttonConnect->setFixedHeight(100);

			auto buttonMoveCenter = new QPushButton("Motor Initialize");
			buttonMoveCenter->setFocusPolicy(Qt::FocusPolicy::NoFocus);
			buttonMoveCenter->setCheckable(true);
			buttonMoveCenter->setFixedWidth(150);
			buttonMoveCenter->setFixedHeight(100);

			auto layoutLabels = new QVBoxLayout;
			{
				auto labelMotorSpeed = new QLabel("Speed\t: ");

				layoutLabels->addWidget(labelMotorSpeed);

				for (int i = 0; i < numMotors; i++)
				{
					auto labelMotorPosition = new QLabel(QString("Motor %1\t: ").arg(i + 1));

					layoutLabels->addWidget(labelMotorPosition);
				}
			}

			auto layoutValues = new QVBoxLayout;
			{
				auto valueMotorSpeed = new QLabel(QString::number(speed));
				valueMotorSpeed->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

				layoutValues->addWidget(valueMotorSpeed);

				for (int i = 0; i < numMotors; i++)
				{
					auto valueMotorPosition = new QLabel("0");
					valueMotorPosition->setObjectName(QString("valueMotorPosition%1").arg(i));
					valueMotorPosition->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
					valueMotorPosition->setFixedWidth(100);

					layoutValues->addWidget(valueMotorPosition);
				}
			}

			connect(buttonConnect, &QPushButton::toggled, [this, buttonConnect](bool checked)
			{
				if (checked)
				{
					bool connect = motor.connect(portName, numMotors);

					if (connect == false)
					{
						buttonConnect->setChecked(false);

						return;
					}

					motor.setPosition(angle, 1, -1, false);
					motor.setPosition(-angle, 2, -1, false);

					motor.setSpeed(speed, 0);
					motor.setSpeed(speed, 1);
					motor.setSpeed(speed, 2);
				}
				else
				{
					motor.disconnect();
				}
			});

			connect(buttonMoveCenter, &QPushButton::clicked, [this](bool checked)
			{
				if (checked)
				{
					for (int i = 0; i < numMotors; i++)
					{
						motor.setPosition(centerPositions[i] * sign, 0, i);
					}
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
				}
			});

			auto layout = new QHBoxLayout;
			layout->setAlignment(Qt::AlignLeft);
			layout->addWidget(buttonConnect);
			layout->addWidget(buttonMoveCenter);
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

			auto buttonStart = new QPushButton("Start");
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

					motionTimer->start();
				}
				else
				{
					buttonStart->setText("Start");

					motionSource->stop();

					motionTimer->stop();
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
			loadOption();
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

	if (loadFile.open(QIODevice::ReadOnly))
	{
		QJsonDocument doc = QJsonDocument::fromJson(loadFile.readAll());
		if (doc.isNull())
		{
			retval = false;
		}
		else
		{
			QJsonObject optionObject = doc.object();

			angle = optionObject["angle"].toInt();

			QJsonArray optionArray = optionObject["motors"].toArray();
			{
				numMotors = optionArray.size();
				centerPositions.resize(numMotors);
				limitPositions.resize(numMotors);

				int index = 0;
				for (auto it = optionArray.begin(); it != optionArray.end(); ++it)
				{
					QJsonObject object = it->toObject();
					centerPositions[index] = object["offset"].toInt();
					limitPositions[index] = object["limit"].toInt();

					index++;
				}
			}

			portName = optionObject["port"].toString();
			sign = optionObject["sign"].toInt();
			speed = optionObject["speed"].toInt();
		}

		loadFile.close();
	}
	else
	{
		printf("ERROR: loatOption() failed.\n");
		retval = false;
	}

	return retval;
}

bool Dialog::saveOption()
{
	QString filepath = QCoreApplication::applicationDirPath();
	QFile saveFile(filepath + "/option.txt");

	QJsonDocument doc;
	QJsonObject optionObject;
	QJsonArray optionArray;

	for (int i = 0; i < numMotors; i++)
	{
		QJsonObject object;
		object["offset"] = centerPositions[i];
		object["limit"] = limitPositions[i];

		optionArray.insert(i, object);
	}

	optionObject["angle"] = angle;
	optionObject["motors"] = optionArray;
	optionObject["port"] = portName;
	optionObject["sign"] = sign;
	optionObject["speed"] = speed;

	if (saveFile.open(QIODevice::WriteOnly | QIODevice::Truncate) == false)
		return false;

	doc.setObject(optionObject);
	saveFile.write(doc.toJson(QJsonDocument::JsonFormat::Indented));
	saveFile.close();

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

	// dynamic load
	

	listMotionSource->setCurrentRow(0);
}
