#ifndef VIRTUALBOXBRIDGE_H
#define VIRTUALBOXBRIDGE_H

#define RT_OS_LINUX
#define VBOX_WITH_XPCOM
#define IN_RING3

#include <nsEventQueueUtils.h>
#include <nsIServiceManager.h>
#include "VirtualBox_XPCOM.h"

#include <QString>
#include <vector>

class VirtualBoxBridge
{
	public:
		VirtualBoxBridge();
		~VirtualBoxBridge();
		QString getVBoxVersion();
		QString generateMac();
		nsCOMPtr<IVirtualBox> virtualBox;
		std::vector<IMachine*> getMachines();

	private:
		bool initXPCOM();
		bool initVirtualBox();
		
		nsCOMPtr<nsIServiceManager> nsCOM_serviceManager;
		nsCOMPtr<nsIEventQueue> nsCOM_eventQ;
		nsCOMPtr<nsIComponentManager> nsCOM_manager;
};

class MachineBridge
{
	public:
		static QString getName(IMachine *machine);
		static std::vector<INetworkAdapter*> getNetworkInterfaces(IVirtualBox *virtualbox, IMachine *machine);
		static bool getIfaceEnabled(INetworkAdapter *iface);
		static QString getIfaceMac(INetworkAdapter* iface);
		static bool setIfaceMac(INetworkAdapter *iface, QString qMac);
};

#endif //VIRTUALBOXBRIDGE_H
