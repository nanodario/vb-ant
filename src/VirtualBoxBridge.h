/*
 * VBANT - VirtualBox Advanced Network Tool
 * Copyright (C) 2015  Dario Messina
 *
 * This file is part of VBANT
 *
 * VBANT is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * VBANT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef VIRTUALBOXBRIDGE_H
#define VIRTUALBOXBRIDGE_H

#define RT_OS_LINUX
#define VBOX_WITH_XPCOM
#define IN_RING3

#include <stdlib.h>
#include <nsEventQueueUtils.h>
#include <nsIServiceManager.h>
#include <nsString.h>

#include "VirtualBox_XPCOM.h"
#include "Iface.h"
#include "UIMainEventListener.h"

#include <QObject>
#include <QString>
#include <vector>
#include <pthread.h>

static QString returnQStringValue(nsXPIDLString s)
{
	const char *str = ToNewCString(s);
	QString retVal = QString::fromAscii(str);
	free((void*)str);
	return retVal;
}

#ifdef DEBUG_FLAG
	#define NS_CHECK_AND_DEBUG_ERROR(ptr, func, out_rc_value) \
	do { \
		out_rc_value = ptr->func; \
		if (NS_FAILED(out_rc_value)) \
		{ \
			std::cerr << "[" << __FILE__ << ":" << __LINE__ << " -> " << __func__ << "] " << #ptr << "->" << #func << ": 0x" << std::hex << out_rc_value << std::dec << std::endl; \
		} \
	} while (0)
#else
	#define NS_CHECK_AND_DEBUG_ERROR(ptr, func, out_rc_value) \
	do { out_rc_value = ptr->func; } while (0)
#endif

#ifdef DEBUG_FLAG
	#define GET_AND_DEBUG_MACHINE_STATE(__session__, __state__, __rc__) \
	do { \
		NS_CHECK_AND_DEBUG_ERROR(__session__, GetState(&__state__), __rc__); \
		if(NS_SUCCEEDED(__rc__)) \
		{ \
			std::cout << "[" << __FILE__ << ":" << __LINE__ << " -> " << __func__ << "] [" << getName().toStdString() << "] SessionState: "; \
			switch(__state__) \
			{ \
				case SessionState::Locked:	std::cout << "Locked!";	   break; \
				case SessionState::Null:	std::cout << "Null!";	   break; \
				case SessionState::Spawning:	std::cout << "Spawning!";  break; \
				case SessionState::Unlocked:	std::cout << "Unlocked!";  break; \
				case SessionState::Unlocking:	std::cout << "Unlocking!"; break; \
			} \
			std::cout << std::endl; \
		} \
	} while (0)
#else
	#define GET_AND_DEBUG_MACHINE_STATE(__session__, __state__, __rc__) \
	do { NS_CHECK_AND_DEBUG_ERROR(__session__, GetState(&__state__), __rc__); } while (0)
#endif
		
#define PRINT_NETWORK_ATTACHMENT_TYPE(attachmentType, prefix) \
	do { \
		std::cout << prefix << "NetworkAttachmentType::"; \
		switch(attachmentType) \
		{ \
			case NetworkAttachmentType::Null:	std::cout << "Null";	   break; \
			case NetworkAttachmentType::Bridged:	std::cout << "Bridged";	   break; \
			case NetworkAttachmentType::Generic:	std::cout << "Generic";	   break; \
			case NetworkAttachmentType::HostOnly:	std::cout << "HostOnly";   break; \
			case NetworkAttachmentType::Internal:	std::cout << "Internal";   break; \
			case NetworkAttachmentType::NAT:	std::cout << "NAT";	   break; \
			case NetworkAttachmentType::NATNetwork:	std::cout << "NATNetwork"; break; \
			default: std::cout << attachmentType << " (unknown type)"; break; \
		} \
		std::cout << std::endl; \
	} while (0)
		
class MachineBridge;
class VMTabSettings;
class VirtualMachine;
class UIMainEventListener;

typedef struct
{
	nsCOMPtr<IVirtualBox> virtualBox;
} tparam_t;

static void *knockAPI(void *tparam);

class VirtualBoxBridge
{
	friend class MachineBridge;
	friend class VirtualMachine;

	public:
		VirtualBoxBridge();
		~VirtualBoxBridge();
		QString getVBoxVersion();
		QString generateMac();
		std::vector<MachineBridge*> getMachines(QObject *parent);
		nsCOMPtr<ISession> newSession();
		nsCOMPtr<IHost> getHost();
		std::vector<nsCOMPtr<IHostNetworkInterface> > getHostNetworkInterfaces();
		std::vector<nsCOMPtr<IHostNetworkInterface> > getHostOnlyInterfaces();
		std::vector<QString> getGenericDriversList();
		std::vector<QString> getInternalNetworkList();
		std::vector<nsCOMPtr<INATNetwork> > getNatNetworks();
		IMachine *cloneVM(QString name, bool reInitIfaces, IMachine *m);
		bool deleteVM(IMachine *m);

	private:
		bool initXPCOM();
		bool initVirtualBox();
		void startAPIknocking();
		
		nsCOMPtr<IVirtualBox> virtualBox;
		nsCOMPtr<nsIServiceManager> nsCOM_serviceManager;
		nsCOMPtr<nsIEventQueue> nsCOM_eventQ;
		nsCOMPtr<nsIComponentManager> nsCOM_manager;
		pthread_t knockThread;
		tparam_t tparam;
};

/* Wrap the IListener interface around our implementation class. */
typedef ListenerImpl<UIMainEventListener, QObject*> UIMainEventListenerImpl;

class MachineBridge
{
	friend class VirtualMachine;
	friend class UIMainEventListener;
	friend class VMTabSettings;

	public:
		MachineBridge(VirtualBoxBridge *vboxbridge, IMachine *machine, QObject *parent);
		~MachineBridge();
		
		uint32_t getMaxNetworkAdapters();
		QString getUUID();
		QString getHardDiskFilePath();
		QString getName();
		uint32_t getState();
		uint32_t getSessionState();
		bool supportsACPI();
		
		//Getters
		std::vector<nsCOMPtr<INetworkAdapter> > getNetworkInterfaces();
		static bool getIfaceEnabled(INetworkAdapter *iface);
		QString getIfaceMac(INetworkAdapter *iface);
		bool getIfaceCableConnected(INetworkAdapter *iface);
		QString getIfaceMac(int iface);
		QString getIfaceFormattedMac(int iface);
		QString getIfaceFormattedMac(INetworkAdapter *iface);
		uint32_t getAttachmentType(INetworkAdapter *iface);
		QString getAttachmentData(INetworkAdapter *iface, uint32_t attachmentType = -1);
		QString getAttachmentData(uint32_t iface, uint32_t attachmentType);
		
		QString getNatNetwork(uint32_t iface);
		QString getBridgedIface(uint32_t iface);
		QString getInternalName(uint32_t iface);
		QString getHostIface(uint32_t iface);
		QString getGenericDriver(uint32_t iface);
		
		//Setters
		bool setIfaceEnabled(uint32_t iface, bool enabled);
		bool setIfaceMac(uint32_t iface, QString qMac);
		bool setIfaceAttachmentType(uint32_t iface, uint32_t attachmentType);
		bool setIfaceAttachmentTypeRunTime(uint32_t iface, uint32_t attachmentType);
		bool setCableConnected(uint32_t iface, bool connected);
		bool setCableConnectedRunTime(uint32_t iface, bool connected);
		bool setAttachmentData(uint32_t iface, QString qAttachmentData);
		bool setAttachmentDataRunTime(uint32_t iface, QString qAttachmentData);
		bool setAttachmentData(uint32_t iface, uint32_t AttachmentType, QString qAttachmentData);
		
		bool start();
		bool stop(bool force = false);
		bool pause(bool pauseEnabled);
		bool reset();

		bool openSettings();
		bool saveSettings();
		
	private:
		bool shutdownVMProcess();
		bool registerListener();
		bool lockMachine();
		bool unlockMachine();
		ComPtr<INetworkAdapter> getIface(uint32_t iface);
		ComPtr<INetworkAdapter> getIfaceRunTimeEditable(uint32_t iface);
		
		QString getNatNetwork(INetworkAdapter *iface);
		QString getBridgedIface(INetworkAdapter *iface);
		QString getInternalName(INetworkAdapter *iface);
		QString getHostIface(INetworkAdapter *iface);
		QString getGenericDriver(INetworkAdapter *iface);
		
		bool setIfaceEnabled(ComPtr<INetworkAdapter> iface, bool enabled);
		bool setIfaceMac(ComPtr<INetworkAdapter> iface, QString qMac);
		bool setIfaceAttachmentType(ComPtr<INetworkAdapter> iface, uint32_t attachmentType);
		bool setCableConnected(ComPtr<INetworkAdapter> iface, bool connected);

		bool setAttachmentData(INetworkAdapter *iface, uint32_t AttachmentType, QString qAttachmentData);
		bool setNatNetwork(ComPtr<INetworkAdapter> iface, QString qNatNetwork);
		bool setBridgedIface(ComPtr<INetworkAdapter> iface, QString qBridgedIface);
		bool setInternalName(ComPtr<INetworkAdapter> iface, QString qInternalName);
		bool setHostIface(ComPtr<INetworkAdapter> iface, QString qHostIface);
		bool setGenericDriver(ComPtr<INetworkAdapter> iface, QString qGenericDriver);
				
		VirtualBoxBridge *vboxbridge;
		IMachine *machine;
		nsCOMPtr<ISession> session;
		nsCOMPtr<IEventSource> eventSource;
		ComObjPtr<UIMainEventListenerImpl> eventListener;
		nsCOMPtr<IConsole> console;
		nsXPIDLString machineUUID;
};

#endif //VIRTUALBOXBRIDGE_H
