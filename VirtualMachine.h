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

#ifndef VIRTUALMACHINE_H
#define VIRTUALMACHINE_H

#include <QString>
#include <QWidget>
#include <vector>
#include <iostream>
#include "Iface.h"
#include "VirtualBoxBridge.h"

typedef enum
{
	IFACE_ENABLED,
	IFACE_MAC,
	IFACE_CONNECTED,
	IFACE_NAME,
#ifdef CONFIGURABLE_IP
	IFACE_IP,
	IFACE_SUBNETMASK,
#endif
	IFACE_ATTACHMENT_TYPE,
	IFACE_SUBNETNAME
} ifacekey_t;

class VirtualMachine
{
	public:
		VirtualMachine(MachineBridge *machine);
		~VirtualMachine();

		bool mountVM(bool readonly = false);
		bool umountVM();
		bool start() const { return machine->start(); };
		bool ACPIstop() const { return machine->stop(); };
		bool stop() const { return machine->stop(true); };
		bool enterPause() const { return machine->pause(true); };
		bool exitPause() const { return machine->pause(false); };
		bool reset() const { return machine->reset(); };
		bool openSettings() const { return machine->openSettings(); };
		bool saveSettings();
		bool loadSettings(QString filename);

#ifdef CONFIGURABLE_IP
		int setIface(int iface, QString name = "", QString mac = "", QString ip = "", QString subnetMask = "");
#else
		int setIface(int iface, QString name = "", QString mac = "");
#endif
		
		bool setNetworkAdapterData(int iface, ifacekey_t key, void *value_ptr);

		Iface *getIfaceByName(QString name);
		Iface *getIfaceByMAC(QString mac);
		Iface *getIfaceByNetworkAdapter(INetworkAdapter *iface);
		Iface **getIfaces() const { return ifaces; };
		void refreshIface(INetworkAdapter *iface);
		QString getIp(uint32_t iface); // const { return QString("10.10.10.%1").arg(iface); }; //TODO
		QString getSubnetMask(uint32_t iface); // const { return QString("%1").arg(iface); }; //TODO
		bool operator==(VirtualMachine *vm);
		void operator=(const VirtualMachine &vm);

	private:
		MachineBridge *machine;
		uint8_t ifaces_size;
		Iface **ifaces;
};

#endif //VIRTUALMACHINE_H
