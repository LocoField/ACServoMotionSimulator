#include "stdafx.h"
#include "Dialog.h"

using namespace std;

int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	Dialog dialog;

	return dialog.exec();
}
