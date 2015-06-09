#include <QtGui/QApplication>
#include "MainWindow.h"

#include <QString>
#include "VirtualBoxBridge.h"

int main(int argc, char** argv)
{
	VirtualBoxBridge *vboxbridge;
	
	vboxbridge = new VirtualBoxBridge();
	QString s = vboxbridge->getVBoxVersion();
	
	printf("VirtualBox version: %s\n", s.toStdString().c_str());
	
	return 0;
	
	QApplication app(argc, argv);
	QStringList args = QApplication::arguments();
	
	MainWindow mw((args.count() < 2) ? QString() : args[1]);
	mw.show();
	
	return app.exec();
}
