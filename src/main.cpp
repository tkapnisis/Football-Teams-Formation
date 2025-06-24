#include "pootjeover.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	PootjeOver w;
	w.show();

	return a.exec();
}
