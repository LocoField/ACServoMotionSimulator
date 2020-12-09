#include "stdafx.h"
#include "ACServoMotionKeyboard.h"

char* ACServoMotionKeyboard::getMotionName()
{
	return "Keyboard";
}

bool ACServoMotionKeyboard::process(void* arg)
{
	Vector3 angleOld = angle;

	angle.x = rollMoved;
	angle.y = pitchMoved;

	if (angleOld == angle)
		return false;

	return true;
}

bool ACServoMotionKeyboard::eventFilter(QObject* object, QEvent* event)
{
	QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
	if (keyEvent)
	{
		if (keyEvent->isAutoRepeat())
		{
			return true;
		}

		if (keyEvent->type() == QEvent::KeyPress)
		{
			switch (keyEvent->key())
			{
				case Qt::Key_W:
				{
					break;
				}
				case Qt::Key_S:
				{
					break;
				}
				case Qt::Key_Left:
				{
					if (rollMoved == 0)
						rollMoved = -1;

					break;
				}
				case Qt::Key_Right:
				{
					if (rollMoved == 0)
						rollMoved = 1;

					break;
				}
				case Qt::Key_Up:
				{
					if (pitchMoved == 0)
						pitchMoved = -1;

					break;
				}
				case Qt::Key_Down:
				{
					if (pitchMoved == 0)
						pitchMoved = 1;

					break;
				}
			}
		}
		else if (keyEvent->type() == QEvent::KeyRelease)
		{
			switch (keyEvent->key())
			{
				case Qt::Key_W:
				{
					break;
				}
				case Qt::Key_S:
				{
					break;
				}
				case Qt::Key_Left:
				{
					if (rollMoved == -1)
						rollMoved = 0;

					break;
				}
				case Qt::Key_Right:
				{
					if (rollMoved == 1)
						rollMoved = 0;

					break;
				}
				case Qt::Key_Up:
				{
					if (pitchMoved == -1)
						pitchMoved = 0;

					break;
				}
				case Qt::Key_Down:
				{
					if (pitchMoved == 1)
						pitchMoved = 0;

					break;
				}
			}
		}

		return true;
	}

	return __super::eventFilter(object, event);
}
