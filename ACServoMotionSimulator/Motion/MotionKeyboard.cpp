#include "MotionKeyboard.h"

constexpr int angle = 15;

char* MotionKeyboard::getMotionName()
{
	return "Keyboard";
}

bool MotionKeyboard::process(void* arg)
{
	motion_.roll = rollMoved;
	motion_.pitch = pitchMoved;

	return true;
}

bool MotionKeyboard::eventFilter(QObject* object, QEvent* event)
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
						rollMoved = -angle;

					break;
				}
				case Qt::Key_Right:
				{
					if (rollMoved == 0)
						rollMoved = angle;

					break;
				}
				case Qt::Key_Up:
				{
					if (pitchMoved == 0)
						pitchMoved = -angle;

					break;
				}
				case Qt::Key_Down:
				{
					if (pitchMoved == 0)
						pitchMoved = angle;

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
					if (rollMoved == -angle)
						rollMoved = 0;

					break;
				}
				case Qt::Key_Right:
				{
					if (rollMoved == angle)
						rollMoved = 0;

					break;
				}
				case Qt::Key_Up:
				{
					if (pitchMoved == -angle)
						pitchMoved = 0;

					break;
				}
				case Qt::Key_Down:
				{
					if (pitchMoved == angle)
						pitchMoved = 0;

					break;
				}
			}
		}

		return true;
	}

	return __super::eventFilter(object, event);
}
