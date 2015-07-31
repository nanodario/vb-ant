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

#include <nsEventQueueUtils.h>
#include <nsIServiceManager.h>
#include <nsString.h>

#include "VirtualBox_XPCOM.h"
#include <obsolete/protypes.h>
#include "Iface.h"
#include <VBox/com/ptr.h>

#include <QString>
#include <vector>

static QString returnQStringValue(nsXPIDLString s);

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
		
		uint32_t getMaxNetworkAdapters();
		QString getUUID();
		QString getHardDiskFilePath();
		QString getName();
		uint32_t getState();

		//Getters
		std::vector<nsCOMPtr<INetworkAdapter> > getNetworkInterfaces();
		static bool getIfaceEnabled(INetworkAdapter *iface);
		QString getIfaceMac(INetworkAdapter *iface);
		QString getIfaceMac(int iface);
		uint32_t getAttachmentType(INetworkAdapter *iface);
		
		//Setters
		bool setIfaceMac(uint32_t iface, QString qMac);
		bool enableIface(uint32_t iface, uint32_t attachmentType);
		bool disableIface(uint32_t iface);
		
		bool start();
		bool stop(bool force = false);
		bool pause();
		bool reset();

		bool openSettings();
		bool saveSettings();
		bool lockMachine();
		bool unlockMachine();

	private:
		ComPtr<INetworkAdapter> getIface(uint32_t iface);
		bool setIfaceMac(ComPtr<INetworkAdapter> iface, QString qMac);
		bool enableIface(ComPtr<INetworkAdapter> iface, uint32_t attachmentType);
		bool disableIface(ComPtr<INetworkAdapter> iface);
		
		VirtualBoxBridge *vboxbridge;
		IMachine *machine;
		nsCOMPtr<ISession> session;
		nsXPIDLString machineUUID;
};

#endif //VIRTUALBOXBRIDGE_H
