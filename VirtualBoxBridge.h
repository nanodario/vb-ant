#ifndef VIRTUALBOXBRIDGE_H
#define VIRTUALBOXBRIDGE_H

#define RT_OS_LINUX
#define VBOX_WITH_XPCOM
#define IN_RING3

#include <nsEventQueueUtils.h>
#include <nsIServiceManager.h>
#include <nsString.h>

#include "VirtualBox_XPCOM.h"
#include "Iface.h"

#include <QString>
#include <vector>

static QString returnQStringValue(nsXPIDLString s);

class MachineBridge;

class VirtualBoxBridge
{
	friend class MachineBridge;

	public:
		VirtualBoxBridge();
		~VirtualBoxBridge();
		QString getVBoxVersion();
		QString generateMac();
		std::vector<MachineBridge*> getMachines();
		nsCOMPtr<ISession> newSession();

	private:
		bool initXPCOM();
		bool initVirtualBox();
		
		nsCOMPtr<IVirtualBox> virtualBox;
		nsCOMPtr<nsIServiceManager> nsCOM_serviceManager;
		nsCOMPtr<nsIEventQueue> nsCOM_eventQ;
		nsCOMPtr<nsIComponentManager> nsCOM_manager;
};

class MachineBridge
{
	public:
		MachineBridge(VirtualBoxBridge *vboxbridge, IMachine *machine);
		~MachineBridge();
		
		QString getUUID();
		QString getHardDiskFilePath();
		QString getName();
		std::vector<INetworkAdapter*> getNetworkInterfaces();
		static bool getIfaceEnabled(INetworkAdapter *iface);
		QString getIfaceMac(INetworkAdapter *iface);
		QString getIfaceMac(int iface);
		bool setIfaceMac(INetworkAdapter *iface, QString qMac);
		bool setIfaceMac(int iface, QString qMac);
		bool start();
		void stop();
		bool saveSettings();

	private:
		bool progressCompleted();
		VirtualBoxBridge *vboxbridge;
		IMachine *machine;
		nsCOMPtr<ISession> session;
};

#endif //VIRTUALBOXBRIDGE_H
