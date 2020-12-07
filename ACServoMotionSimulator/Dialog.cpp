#include "stdafx.h"
#include "Dialog.h"
#include "ACServoMotorHelper.h"

#include <QtWidgets/QMenuBar>

#define DIALOG_TITLE "Motion Simulator by Hotas 4"

Dialog::Dialog()
{
	initialize();
	loadOption();
}

Dialog::~Dialog()
{
}

void Dialog::initialize()
{
	installEventFilter(this);

	timerUpdateUI = new QTimer;
	connect(timerUpdateUI, &QTimer::timeout, [this]()
	{
		if (needUpdateUI)
			updateUI();

		

		needUpdateUI = false;
	});

	{
		auto groupBox = new QGroupBox("Motor");

		motorLayout = new QVBoxLayout;
		motorLayout->addWidget(groupBox);

		{
			auto buttonConnect = new QPushButton("Connect");
			buttonConnect->setFocusPolicy(Qt::FocusPolicy::NoFocus);
			buttonConnect->setCheckable(true);
			buttonConnect->setFixedWidth(100);
			buttonConnect->setFixedHeight(100);

			auto buttonMoveCenter = new QPushButton("Motor\n\nInitialize");
			buttonMoveCenter->setFocusPolicy(Qt::FocusPolicy::NoFocus);
			buttonMoveCenter->setCheckable(true);
			buttonMoveCenter->setFixedWidth(100);
			buttonMoveCenter->setFixedHeight(100);

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

					motor.setSpeed(speed);
				}
				else
				{
					motor.disconnect();

					numMotors = 0;
				}
			});

			connect(buttonMoveCenter, &QPushButton::clicked, [this](bool checked)
			{
				if (checked)
				{
					for (int i = 0; i < numMotors; i++)
					{
						motor.setPosition(centerPositions[i] * sign, i);
					}
				}
				else
				{
					for (int i = 0; i < numMotors;)
					{
						int position = 0;
						bool moving = true;

						motor.position(i, position, moving);
						position *= sign;

						if (moving)
							continue;

						motor.setPosition(-position, i);

						i++;
					}
				}
			});

			auto layout = new QHBoxLayout;
			layout->setAlignment(Qt::AlignLeft);
			layout->addWidget(buttonConnect);
			layout->addWidget(buttonMoveCenter);

			groupBox->setLayout(layout);
		}
	}

	{
		auto groupBox = new QGroupBox("Controller");

		controllerLayout = new QVBoxLayout;
		controllerLayout->addWidget(groupBox);

		{
			auto buttonStart = new QPushButton("Start");
			buttonStart->setFocusPolicy(Qt::FocusPolicy::NoFocus);
			buttonStart->setCheckable(true);
			buttonStart->setFixedWidth(100);
			buttonStart->setFixedHeight(100);

			connect(buttonStart, &QPushButton::toggled, [this, buttonStart](bool checked)
			{
				if (checked)
				{
					timerUpdateUI->start();
				}
				else
				{
					timerUpdateUI->stop();
				}
			});

			auto layout = new QHBoxLayout;
			layout->setAlignment(Qt::AlignLeft);
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
		QAction* actionLoadOption = new QAction("Load Option");
		connect(actionLoadOption, &QAction::triggered, [&]()
		{
			loadOption();
		});

		menuView->addAction(actionLoadOption);
		menu->addMenu(menuView);
	}
}

void Dialog::updateUI()
{
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

bool Dialog::eventFilter(QObject* object, QEvent* event)
{
	QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
	if (keyEvent)
	{
		if (keyEvent->key() == Qt::Key_Escape)
		{
			return true;
		}

		if (event->type() == QEvent::KeyRelease)
		{
			keyPressEvent(keyEvent);
			return true;
		}
	}

	return __super::eventFilter(object, event);
}

void Dialog::keyPressEvent(QKeyEvent* event)
{
	if (timerUpdateUI->isActive() == false)
	{
		return;
	}

	if (event->isAutoRepeat())
	{
		return;
	}

	int heave = 0;

	if (event->type() == QEvent::KeyRelease)
	{
		switch (event->key())
		{
			case Qt::Key_Left:
			{
				if (rollMoved != -1)
					return;

				rollMoved = 0;

				break;
			}
			case Qt::Key_Right:
			{
				if (rollMoved != 1)
					return;

				rollMoved = 0;

				break;
			}
			case Qt::Key_Up:
			{
				if (pitchMoved != -1)
					return;

				pitchMoved = 0;

				break;
			}
			case Qt::Key_Down:
			{
				if (pitchMoved != 1)
					return;

				pitchMoved = 0;

				break;
			}
		}
	}
	else
	{
		switch (event->key())
		{
			case Qt::Key_W:
			{
				heave = 1;

				printf("heave up\n");

				break;
			}
			case Qt::Key_S:
			{
				heave = -1;

				printf("heave down\n");

				break;
			}
			case Qt::Key_Left:
			{
				if (rollMoved != 0)
					return;

				rollMoved = -1;

				printf("left\n");
				break;
			}
			case Qt::Key_Right:
			{
				if (rollMoved != 0)
					return;

				rollMoved = 1;

				printf("right\n");
				break;
			}
			case Qt::Key_Up:
			{
				if (pitchMoved != 0)
					return;

				pitchMoved = -1;

				printf("up\n");
				break;
			}
			case Qt::Key_Down:
			{
				if (pitchMoved != 0)
					return;

				pitchMoved = 1;

				printf("down\n");
				break;
			}
		}
	}


	if (motor.isConnected() == false)
		return;

	std::vector<int> currentPositions(numMotors);
	int completeCount = 0;

	for (int i = 0; i < numMotors; i++)
	{
		bool moving = true;

		motor.position(i, currentPositions[i], moving);
		currentPositions[i] *= sign;

		printf("%6d    ", currentPositions[i]);

		if (moving == false)
			completeCount++;
	}

	printf("\n");

	if (completeCount != numMotors)
	{
		printf("Motors are moving.\n");

		return;
	}


	// TODO: 모터가 2개인 경우 heave 값은 무시한다.

	std::vector<int> cycleValues(numMotors);

	for (int i = 0; i < numMotors; i++)
	{
		cycleValues[i] += (heave * 10000 * sign);
	}

	if (numMotors == 2)
	{
		std::vector<int> desirePosition = centerPositions;

		desirePosition[0] += (rollMoved * angle * sign);
		desirePosition[1] -= (rollMoved * angle * sign);
		desirePosition[0] -= (pitchMoved * angle * sign);
		desirePosition[1] -= (pitchMoved * angle * sign);

		cycleValues[0] += (desirePosition[0] - currentPositions[0]);
		cycleValues[1] += (desirePosition[1] - currentPositions[1]);
	}
	else if (numMotors == 4)
	{
		std::vector<int> desirePosition = centerPositions;

		desirePosition[0] += (rollMoved * angle * sign);
		desirePosition[1] += (rollMoved * angle * sign);
		desirePosition[2] -= (rollMoved * angle * sign);
		desirePosition[3] -= (rollMoved * angle * sign);

		desirePosition[0] += (pitchMoved * angle * sign);
		desirePosition[1] += (pitchMoved * angle * sign);
		desirePosition[2] -= (pitchMoved * angle * sign);
		desirePosition[3] -= (pitchMoved * angle * sign);

		cycleValues[0] += (desirePosition[0] - currentPositions[0]);
		cycleValues[1] += (desirePosition[1] - currentPositions[1]);
		cycleValues[2] += (desirePosition[2] - currentPositions[2]);
		cycleValues[3] += (desirePosition[3] - currentPositions[3]);
	}

	for (int i = 0; i < numMotors; i++)
	{
		int position = cycleValues[i];

		if (currentPositions[i] + position < 0 ||
			currentPositions[i] + position >= limitPositions[i])
			position = 0;

		motor.setPosition(position, i, false);
	}

	motor.trigger();
}

void Dialog::closeEvent(QCloseEvent* event)
{
	__super::closeEvent(event);
}
