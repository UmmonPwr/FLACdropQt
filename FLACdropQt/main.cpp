#include "FLACdropQt.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	FLACdropQt w;
	w.show();
	return a.exec();
}
