#include "stdafx.h"
#include "Dialog.h"

#include "Motion/MotionBase.h"
#include "Motion/MotionKeyboard.h"
#include "Motion/MotionPCars2.h"
#include "Motion/MotionNoLimits2.h"
#include "Motion/MotionXPlane11.h"

#define DIALOG_TITLE "LocoField Motion Simulator v2.3"

Dialog::Dialog()
{
	bool retval = loadOption();

	initialize();

	addMotionModules();

	if (retval == false)
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
		Motion data;

		if (motionSource)
		{
			if (motionSource->process(this) == false)
				return;

			motionSource->motion(data);
		}

#ifdef _DEBUG
		printf("%2.5f    %2.5f    %2.5f\n%2.5f    %2.5f    %2.5f\n%2.5f    %2.5f    %2.5f    %2.5f\n%u\n",
			data.roll, data.pitch, data.yaw,
			data.sway, data.surge, data.heave,
			data.ll, data.lr, data.rl, data.rr,
			GetTickCount());
#endif

		controller.motion(data);
		Sleep(30);
	});

	auto controllerLayout = new QVBoxLayout;
	{
		auto groupBox = new QGroupBox("Controller");
		controllerLayout->addWidget(groupBox);

		{
			auto buttonMotorConnect = new QPushButton("Connect");
			buttonMotorConnect->setFocusPolicy(Qt::FocusPolicy::NoFocus);
			buttonMotorConnect->setCheckable(true);
			buttonMotorConnect->setFixedWidth(150);
			buttonMotorConnect->setFixedHeight(100);

			auto buttonMotorStart = new QPushButton("Start");
			buttonMotorStart->setFocusPolicy(Qt::FocusPolicy::NoFocus);
			buttonMotorStart->setCheckable(true);
			buttonMotorStart->setFixedWidth(150);
			buttonMotorStart->setFixedHeight(100);

			connect(buttonMotorConnect, &QPushButton::toggled, [this, buttonMotorConnect](bool checked)
			{
				if (checked)
				{
					if (controller.connect(controllerPortName.toStdString(), baudRate) == false)
					{
						buttonMotorConnect->setChecked(false);
						return;
					}

					buttonMotorConnect->setText("Disconnect");
				}
				else
				{
					controller.disconnect();

					buttonMotorConnect->setText("Connect");
				}
			});

			connect(buttonMotorStart, &QPushButton::clicked, [this, buttonMotorStart](bool checked)
			{
				if (checked)
				{
					controller.start();

					buttonMotorStart->setText("Stop");
				}
				else
				{
					controller.stop();

					buttonMotorStart->setText("Start");
				}
			});

			auto layout = new QHBoxLayout;
			layout->setAlignment(Qt::AlignLeft);
			layout->addWidget(buttonMotorConnect);
			layout->addWidget(buttonMotorStart);

			groupBox->setLayout(layout);
		}
	}

	auto motionLayout = new QVBoxLayout;
	{
		auto groupBox = new QGroupBox("Motion");
		motionLayout->addWidget(groupBox);

		{
			auto listMotionSource = new QListWidget;
			listMotionSource->setObjectName("listMotionSource");
			listMotionSource->setFixedWidth(300);
			listMotionSource->setFixedHeight(120);

			auto buttonStart = new QPushButton("Start");
			buttonStart->setFocusPolicy(Qt::FocusPolicy::NoFocus);
			buttonStart->setCheckable(true);
			buttonStart->setFixedWidth(150);
			buttonStart->setFixedHeight(120);

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
					buttonStart->setText("Start");

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
	mainLayout->addLayout(controllerLayout);
	mainLayout->addLayout(motionLayout);

	mainWidget = new QWidget;
	mainWidget->setLayout(mainLayout);

	mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(mainWidget);


	setWindowTitle(DIALOG_TITLE);
	setWindowFlag(Qt::WindowMinimizeButtonHint);


	QMenuBar* menu = new QMenuBar();
	layout()->setMenuBar(menu);

	QMenu* menuAction = new QMenu("Action");
	{
		QAction* actionPowerOff = new QAction("Power Off");

		connect(actionPowerOff, &QAction::triggered, [this]()
		{
			controller.power(false);
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

bool Dialog::loadOption()
{
	int width = 500;
	int height = 1000;
	int center = 0;
	int limit = 0;

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

			if (optionList.contains("default"))
			{
				QJsonObject defaultOption = optionObject["default"].toObject();
				auto defaultOptionList = defaultOption.keys();

				if (defaultOptionList.contains("width") &&
					defaultOptionList.contains("height") &&
					defaultOptionList.contains("center") &&
					defaultOptionList.contains("limit") &&
					defaultOptionList.contains("port"))
				{
					auto itBaudRate = defaultOption.find("baudRate");
					if (itBaudRate != defaultOption.end())
						baudRate = itBaudRate->toInt();

					width = defaultOption["width"].toInt();
					height = defaultOption["height"].toInt();
					center = defaultOption["center"].toInt();
					limit = defaultOption["limit"].toInt();
					controllerPortName = defaultOption["port"].toString();
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

				controller.setMotionSize(width, height);
				controller.setCenterPosition(center);
				controller.setLimitPosition(limit);
			}
			else
			{
				retval = false;
			}
		}

		loadFile.close();
	}

	if (retval == false)
	{
		printf("ERROR: loadOption() failed.\n");
		return false;
	}

	return true;
}

void Dialog::addMotionModules()
{
	auto listMotionSource = findChild<QListWidget*>("listMotionSource");

	auto addModule = [this, listMotionSource](MotionBase* motion)
	{
		listMotionSource->addItem(motion->getMotionName());

		motionSources.emplace_back(motion);
	};

	// default module
	MotionKeyboard* motionKeyboard = new MotionKeyboard;
	{
		installEventFilter(motionKeyboard); // for receiving QKeyEvent

		addModule(motionKeyboard);
	}

	MotionPCars2* motionPCars2 = new MotionPCars2;
	{
		addModule(motionPCars2);
	}

	MotionNoLimits2* motionNoLimits2 = new MotionNoLimits2;
	{
		addModule(motionNoLimits2);
	}

	MotionXPlane11* motionXPlane11 = new MotionXPlane11;
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
