/*
#define RT_OS_LINUX
#define VBOX_WITH_XPCOM

#include <nsMemory.h>
#include <nsString.h>
#include <nsIServiceManager.h>
#include <nsEventQueueUtils.h>

#include <nsIExceptionService.h>

#include <VBox/com/com.h>
#include <VBox/com/string.h>
#include <VBox/com/array.h>
#include <VBox/com/Guid.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>

#include <VBox/com/VirtualBox.h>

#include <iprt/stream.h>


// VirtualBox XPCOM interface. This header is generated
// from IDL which in turn is generated from a custom XML format.

#include "VirtualBox_XPCOM.h"

#define RTStrPrintf(...) printf("RTStrPrintf (%d)\n", __LINE__)
#define RTPrintf(...) printf("RTPrintf (%d)\n", __LINE__)
*/

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
