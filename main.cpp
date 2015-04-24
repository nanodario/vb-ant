#include <QtGui/QApplication>
#include "MainWindow.h"

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	QStringList args = QApplication::arguments();
	
	MainWindow mw((args.count() < 2) ? QString() : args[1]);
	mw.show();
	
	return app.exec();
}
