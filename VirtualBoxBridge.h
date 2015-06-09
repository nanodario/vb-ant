#ifndef VIRTUALBOXBRIDGE_H
#define VIRTUALBOXBRIDGE_H

#include <QString>

#define RT_OS_LINUX
#define VBOX_WITH_XPCOM
#define IN_RING3

#include <nsEventQueueUtils.h>
#include <nsIServiceManager.h>
#include "VirtualBox_XPCOM.h"

class VirtualBoxBridge
{
	public:
		VirtualBoxBridge();
		~VirtualBoxBridge();
		QString getVBoxVersion();
		QString generateMac();

	private:
		bool initXPCOM();
		bool initVirtualBox();
		
		nsCOMPtr<nsIServiceManager> nsCOM_serviceManager;
		nsCOMPtr<nsIEventQueue> nsCOM_eventQ;
		nsCOMPtr<nsIComponentManager> nsCOM_manager;
		nsCOMPtr<IVirtualBox> virtualBox;
};

#endif //VIRTUALBOXBRIDGE_H
