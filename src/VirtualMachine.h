/*
 * VB-ANT - VirtualBox - Advanced Network Tool
 * Copyright (C) 2015, 2016  Dario Messina
 *
 * This file is part of VB-ANT
 *
 * VB-ANT is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * VB-ANT is distributed in the hope that it will be useful,
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
	IFACE_ATTACHMENT_DATA
} ifacekey_t;

class MainWindow;
class VMTabSettings;
class SummaryDialog;
class VMSettings;

class VirtualMachine : QObject
{
	friend class MainWindow;
	friend class VMTabSettings;
	friend class SummaryDialog;
	friend class VMSettings;

	Q_OBJECT;

	public:
		VirtualMachine(MachineBridge *machine, std::string vhd_mountpoint, std::string partition_mountpoint_prefix);
		~VirtualMachine();

		bool mountVpartition(int index, bool readonly = false);
		bool umountVpartition(int index);
		bool start();
		bool ACPIstop() const { return machine->stop(); };
		bool stop() const { return machine->stop(true); };
		bool enterPause() const { return machine->pause(true); };
		bool exitPause() const { return machine->pause(false); };
		bool reset() const { return machine->reset(); };
		bool shutdownVMProcess() const { return machine->shutdownVMProcess(); };
		bool openSettings() const { return machine->openSettings(); };
		IMachine *clone(QString qName, bool reInitIfaces);
		bool remove();
		bool saveSettings();
		bool saveSettingsRunTime();
		bool loadSettings(QString filename);

#ifdef CONFIGURABLE_IP
		int setIface(int iface, QString name = "", QString mac = "", QString ip = "", QString subnetMask = "");
#else
		int setIface(int iface, QString name = "", QString mac = "");
#endif
		void populateIfaces();	
		void cleanIfaces(Iface **ifaces_src, int ifaces_src_size);
		void copyIfaces(Iface **ifaces_src, int ifaces_src_size);
		bool setNetworkAdapterData(int iface, ifacekey_t key, void *value_ptr);

		Iface *getIfaceByName(QString name);
		Iface *getIfaceByMAC(QString mac);
		Iface *getIfaceByNetworkAdapter(INetworkAdapter *iface);
		Iface **getIfaces() const { return ifaces; };
		void refreshIface(INetworkAdapter *iface);
		QString getIfaceName(uint32_t iface);
		QString getIp(uint32_t iface);
		QString getSubnetMask(uint32_t iface);
		
		bool operator==(VirtualMachine *vm);
		void operator=(const VirtualMachine &vm);

	private:
		bool mountVHD();
		bool umountVHD();
		MachineBridge *machine;
		uint8_t ifaces_size;
		Iface **ifaces;
		std::string vhd_mountpoint;
		std::string partition_mountpoint_prefix;
		std::vector<std::string> mounted_partitions_vec;
		bool vhd_mounted;

	signals:
		void settingsChanged(VirtualMachine *vm);
};

#endif //VIRTUALMACHINE_H
