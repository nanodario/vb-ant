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

#include "VirtualMachine.h"

#include <QDomDocument>
#include <QDomElement>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QLabel>

#include <unistd.h>
#include <sys/syscall.h>
#include <malloc.h>
#include <string>
#include <sstream>

#include "VirtualBoxBridge.h"
#include "OSBridge.h"

#define NET_HW_SETTINGS_FILE "etc/udev/rules.d/70-persistent-net.rules"
#define NET_SW_SETTINGS_PREFIX "etc/sysconfig/network-scripts/ifcfg-"

VirtualMachine::VirtualMachine(MachineBridge *machine, std::string vhd_mountpoint, std::string partition_mountpoint_prefix)
: machine(machine), ifaces_size(0), ifaces(NULL), vhd_mountpoint(vhd_mountpoint)
, partition_mountpoint_prefix(partition_mountpoint_prefix)
{
	populateIfaces();
}

VirtualMachine::~VirtualMachine()
{
	while(ifaces_size > 0)
		delete ifaces[ifaces_size-- - 1];
	
	free(ifaces);
	
	while(mounted_partitions_vec.size() > 0)
	{
		int i = mounted_partitions_vec.back().find_last_of('p');
		umountVpartition(atoi(mounted_partitions_vec.back().substr(i).c_str()));
		mounted_partitions_vec.pop_back();
	}
}

bool VirtualMachine::mountVHD()
{
	if(mounted_partitions_vec.size() == 0)
	{
		std::cout << "Mounting " << machine->getHardDiskFilePath().toStdString() << " on " << vhd_mountpoint << std::endl;
		return OSBridge::mountVHD(machine->getHardDiskFilePath().toStdString(), vhd_mountpoint);
	}

	return true;
}

bool VirtualMachine::umountVHD()
{
	while(mounted_partitions_vec.size() > 0)
	{
		int i = mounted_partitions_vec.back().find_last_of('p');
		umountVpartition(atoi(mounted_partitions_vec.back().substr(i).c_str()));
	}

	std::cout << "Unmounting " << machine->getHardDiskFilePath().toStdString() << " from " << vhd_mountpoint << std::endl;
	return OSBridge::umountVHD(vhd_mountpoint);
}

bool VirtualMachine::mountVpartition(int index, bool readonly)
{
	if(mounted_partitions_vec.size() == 0)
		if(!mountVHD())
			return false;

	std::stringstream partition; partition << vhd_mountpoint << "p" << index;
	std::stringstream partition_mountpoint;	partition_mountpoint << partition_mountpoint_prefix << "p" << index;
	std::stringstream partition_usermountpoint; partition_usermountpoint << partition_mountpoint_prefix << "p" << index<< "-u";

	std::cout << "Mounting " << partition.str() << " on " << partition_mountpoint.str() << std::endl;
	for(int i = 0; i < mounted_partitions_vec.size(); i++)
		if(mounted_partitions_vec.at(i) == partition.str())
		{
			std::cout << partition.str() << " already mounted" << std::endl;
			return true;
		}

	if(OSBridge::mountVpartition(partition.str(), partition_mountpoint.str(), partition_usermountpoint.str()), readonly)
	{
		mounted_partitions_vec.push_back(partition.str());
		return true;
	}

	return false;
}

bool VirtualMachine::umountVpartition(int index)
{
	std::stringstream partition; partition << vhd_mountpoint << "p" << index;
	std::stringstream mpoint; mpoint << partition_mountpoint_prefix << "p" << index;
	std::stringstream usermpoint; usermpoint << partition_mountpoint_prefix << "p" << index << "-u";
	
	for(int i = 0; i < mounted_partitions_vec.size(); i++)
	{
		if(mounted_partitions_vec.at(i) == partition.str())
		{
			std::cout << "Unmounting " << partition.str() << std::endl;
			if(OSBridge::umountVpartition(usermpoint.str()) && OSBridge::umountVpartition(mpoint.str()))
			{
				mounted_partitions_vec.erase(mounted_partitions_vec.begin() + i);
				break;
			}
		}
	}

	if(mounted_partitions_vec.size() == 0)
		if(!umountVHD())
			return false;
	
	return true;	
}

bool VirtualMachine::start()
{
	bool succeeded = true;

	if(!machine->lockMachine())
	{
		std::cerr << "[" << machine->getName().toStdString() <<  "] Cannot lock machine" << std::endl;
		return false;
	}

	for(int i = 0; i < ifaces_size; i++)
		if(ifaces[i]->enabled)
			if(!machine->setCableConnected(i, ifaces[i]->cableConnected))
				succeeded = false;

	if(!machine->saveSettings())
	{
		std::cout << "saveCableConnectedSettings(): false" << std::endl;
		succeeded = false;
	}

	if(!machine->unlockMachine())
		return false;

	return machine->start() && succeeded;
}

Iface *VirtualMachine::getIfaceByMAC(QString mac)
{
	for (int i = 0; i < ifaces_size; i++)
		if (ifaces[i]->mac == mac)
			return ifaces[i];
	
	return NULL;
}

Iface *VirtualMachine::getIfaceByNetworkAdapter(INetworkAdapter *iface)
{
	return getIfaceByMAC(machine->getIfaceFormattedMac(iface));
}

Iface *VirtualMachine::getIfaceByName(QString name)
{
	for (int i = 0; i < ifaces_size; i++)
		if (ifaces[i]->name == name)
			return ifaces[i];

	return NULL;
}

void VirtualMachine::refreshIface(INetworkAdapter *iface)
{
	Iface *i = getIfaceByNetworkAdapter(iface);
	if(i == NULL)
		return;

	int iface_index;
	for(iface_index = 0; iface_index < ifaces_size; iface_index++)
		if(ifaces[iface_index] == i)
			break;

	if(iface_index >= ifaces_size)
		return;

	i->enabled = machine->getIfaceEnabled(iface);
	i->setMac(machine->getIfaceMac(iface));
	i->cableConnected = machine->getIfaceCableConnected(iface);
	i->setAttachmentType(machine->getAttachmentType(iface));
	i->setAttachmentData(machine->getAttachmentData(iface, machine->getAttachmentType(iface)));
	i->name = getIfaceName(iface_index);
#ifdef CONFIGURABLE_IP
	i->setIp(getIp(iface_index));
	i->setSubnetMask(getSubnetMask(iface_index));
#endif
}

QString VirtualMachine::getIfaceName(uint32_t iface)
{
	//read from /etc/udev/rules.d/70-persistent-net.rules
	/* .
	 * .
	 * 
	 * # (01:23:45:67:89:0A) iface0
	 * SUBSYSTEM=="net", ACTION=="add", DRIVERS=="?*", ATTR{address}=="01:23:45:67:89:0A", ATTR{dev_id}=="0x0", ATTR{type}=="1", KERNEL=="eth*", NAME="iface0"
	 * 
	 * -
	 * -
	 */
	QString iface_name = QString("noname%1").arg(iface);
	QString match = QString::fromUtf8("{address}==\"").toUpper().append(machine->getIfaceFormattedMac(iface).toUpper()).append("\"");

	QString filename = QString::fromStdString(partition_mountpoint_prefix);
	filename.append(QString::fromUtf8("p%1-u/").arg(OS_PARTITION_NUMBER)).append(NET_HW_SETTINGS_FILE);

	QFile file(filename);

	if(file.open(QIODevice::ReadOnly))
	{
		while(!file.atEnd())
		{
			QString line = file.readLine();
			if(line.toUpper().contains(match))
			{
				int ifacename_index_begin = line.lastIndexOf(QString::fromUtf8("NAME="));
				int ifacename_index_end = QString::fromStdString(line.toStdString().substr(ifacename_index_begin + 6)).lastIndexOf(QString::fromUtf8("\""));
				iface_name = QString::fromStdString(line.toStdString().substr(ifacename_index_begin + 6, ifacename_index_end));
				break;
			}
		}
		file.close();
	}

	return iface_name;
}

QString VirtualMachine::getIp(uint32_t iface)
{
	//read from /etc/sysconfig/network-scripts/ifcfg-IFACE_NAME
	/*
	 * DEVICE=eth0
	 * HWADDR=08:00:27:C9:2D:87
	 * IPADDR=208.164.186.1
	 * NETMASK=255.255.255.0
	 * ONBOOT=yes
	 * BOOTPROTO=none
	 */
	QString iface_ip = QString::fromUtf8("");
	const QString iface_name = getIfaceName(iface);
	const QString iface_mac = machine->getIfaceFormattedMac(iface).toUpper();

	bool correct_name = false;
	bool correct_mac = false;

	QString match_name = QString::fromUtf8("DEVICE=").append(iface_name.toUpper());
	QString match_mac = QString::fromUtf8("HWADDR=").append(iface_mac.toUpper());

	QString filename = QString::fromStdString(partition_mountpoint_prefix);
	filename.append(QString::fromUtf8("p%1-u/").arg(OS_PARTITION_NUMBER)).append(NET_SW_SETTINGS_PREFIX).append(iface_name);

// 	QString filename = QString::fromUtf8("/dev/shm/ifcfg-eth0");
	QFile file(filename);

	if(file.open(QIODevice::ReadOnly))
	{
		QString iface_ip_tmp = QString::fromUtf8("");;
		while(!file.atEnd())
		{
			QString line = file.readLine();
			if(!correct_name && line.toUpper().contains(match_name))
				correct_name = true;
			if(!correct_mac && line.toUpper().contains(match_mac))
				correct_mac = true;

			if(line.toUpper().contains(QString::fromUtf8("IPADDR=")))
			{
				int ifacename_index_begin = line.lastIndexOf(QString::fromUtf8("IPADDR="));
				iface_ip_tmp = (QString::fromStdString(line.toStdString().substr(ifacename_index_begin + 7))).trimmed();
			}
		}

		if(!correct_name)
			std::cerr << "Warning: iface " << iface << " (" << iface_mac.toStdString() << ", " << iface_name.toStdString() << ") does not match the ifcfg-" << iface_name.toStdString() << " settings file" << std::endl;

		if(correct_mac)
			iface_ip = iface_ip_tmp;

		file.close();
	}

	return iface_ip;
}

QString VirtualMachine::getSubnetMask(uint32_t iface)
{
	//read from /etc/sysconfig/network-scripts/ifcfg-IFACE_NAME
	/*
	 * DEVICE=eth0
	 * HWADDR=08:00:27:C9:2D:87
	 * IPADDR=208.164.186.1
	 * NETMASK=255.255.255.0
	 * ONBOOT=yes
	 * BOOTPROTO=none
	 */
	QString iface_subnetMask = QString::fromUtf8("");
	const QString iface_name = getIfaceName(iface);
	const QString iface_mac = machine->getIfaceFormattedMac(iface).toUpper();

	bool correct_name = false;
	bool correct_mac = false;

	QString match_name = QString::fromUtf8("DEVICE=").append(iface_name.toUpper());
	QString match_mac = QString::fromUtf8("HWADDR=").append(iface_mac.toUpper());

	QString filename = QString::fromStdString(partition_mountpoint_prefix);
	filename.append(QString::fromUtf8("p%1-u/").arg(OS_PARTITION_NUMBER)).append(NET_SW_SETTINGS_PREFIX).append(iface_name);

// 	QString filename = QString::fromUtf8("/dev/shm/ifcfg-eth0");
	QFile file(filename);

	if(file.open(QIODevice::ReadOnly))
	{
		QString iface_subnetMask_tmp = QString::fromUtf8("");;
		while(!file.atEnd())
		{
			QString line = file.readLine();
			if(!correct_name && line.toUpper().contains(match_name))
				correct_name = true;
			if(!correct_mac && line.toUpper().contains(match_mac))
				correct_mac = true;
			
			if(line.toUpper().contains(QString::fromUtf8("NETMASK=")))
			{
				int ifacename_index_begin = line.lastIndexOf(QString::fromUtf8("NETMASK="));
				iface_subnetMask_tmp = (QString::fromStdString(line.toStdString().substr(ifacename_index_begin + 8))).trimmed();
			}
		}

		if(correct_mac)
		{
			if(!correct_name)
				std::cerr << "Warning: iface " << iface << " (" << iface_mac.toStdString() << ", " << iface_name.toStdString() << ") does not match the ifcfg-" << iface_name.toStdString() << " settings file" << std::endl;

			iface_subnetMask = iface_subnetMask_tmp;
		}

		file.close();
	}
	
	return iface_subnetMask;
}

IMachine *VirtualMachine::clone(QString qName, bool reInitIfaces)
{
	return machine->vboxbridge->cloneVM(qName, reInitIfaces, machine->machine);
}

bool VirtualMachine::remove()
{
	uint32_t machineState = machine->getState();
	if(machineState == MachineState::Paused ||
	   machineState == MachineState::Starting ||
	   machineState == MachineState::Running)
		return false;

	return machine->vboxbridge->deleteVM(machine->machine);
}

bool VirtualMachine::saveSettings()
{
	bool succeeded = true;
	
	if(!machine->lockMachine())
	{
		std::cerr << "[" << machine->getName().toStdString() <<  "] Cannot lock machine" << std::endl;
		return false;
	}
	
	/*
	 * SET VIRTUALBOX PARAMS
	 */
	for(int i = 0; i < ifaces_size; i++)
	{
		//Interfaccia abilitata
		if(!machine->setIfaceEnabled(i, ifaces[i]->enabled))
		{
			std::cout << (ifaces[i]->enabled ? "enableIface" : "disableIface") << "(" << i << ")" << std::endl;
			succeeded = false;
		}
		
		//Indirizzo MAC
		if(!machine->setIfaceMac(i, ifaces[i]->mac))
		{
			std::cout << "setIfaceMac(" << i << "): false" << std::endl;
			succeeded = false;
		}
		
		if(ifaces[i]->enabled)
		{
			//Collegata
			if(!machine->setCableConnected(i, ifaces[i]->cableConnected))
			{
				std::cout << (ifaces[i]->cableConnected ? "connected" : "not connected") << "(" << i << ")" << std::endl;
				succeeded = false;
			}
		
			//Tipo interfaccia
			if(!machine->setIfaceAttachmentType(i, ifaces[i]->attachmentType))
			{
				std::cout << "setIfaceAttachmentType(" << i << ", " << ifaces[i]->attachmentType << ")" << std::endl;
				succeeded = false;
			}
			
			//Parametro in base al tipo di interfaccia
			if(!machine->setAttachmentData(i, ifaces[i]->attachmentType, ifaces[i]->attachmentData))
			{
				std::cout << "setAttachmentData(" << i << ", " << ifaces[i]->attachmentData.toStdString() << ")" << std::endl;
				succeeded = false;
			}
		}
	}

	mountVpartition(OS_PARTITION_NUMBER);
	/*
	 * SET OS PARAMS
	 */
	QString filename = QString::fromStdString(partition_mountpoint_prefix);
// 	QString filename = QString::fromUtf8("/dev/shm/");
	filename.append(QString::fromUtf8("p%1-u/").arg(OS_PARTITION_NUMBER)).append(NET_HW_SETTINGS_FILE);
	
	QFile hw_file(filename);
	std::cout << "filename: " << filename.toStdString() << std::endl;

	if(hw_file.exists() && hw_file.open(QIODevice::WriteOnly))
	{
		std::cout << "Scrivo sul file " << filename.toStdString() << std::endl;
		hw_file.write("# This file was automatically generated by the " PROGRAM_NAME " program\n#\n# You can modify it, as long as you keep each rule on a single\n# line, and change only the value of the NAME= key.\n\n");

		for(int i = 0; i < ifaces_size; i++)
		{
			if(ifaces[i]->enabled)
			{
				std::string line = "";

				line.append("# (");
				line.append(ifaces[i]->mac.toStdString());
				line.append(") ");
				line.append(ifaces[i]->name.toStdString());
				line.append("\n");
				line.append("SUBSYSTEM==\"net\", ACTION==\"add\", DRIVERS==\"?*\", ATTR{address}==\"");
				line.append(ifaces[i]->mac.toStdString());
				line.append("\", ATTR{type}==\"1\", KERNEL==\"eth*\", NAME=\"");
				line.append(ifaces[i]->name.toStdString());
				line.append("\"\n\n");

				hw_file.write(line.c_str());
				hw_file.flush();
			}
		}
		
		hw_file.close();
	}

	for(int i = 0; i < ifaces_size; i++)
	{
		QString last_filename = QString::fromStdString(partition_mountpoint_prefix).append(QString::fromUtf8("p%1-u/").arg(OS_PARTITION_NUMBER)).append(NET_SW_SETTINGS_PREFIX).append(ifaces[i]->last_valid_name);
		if(QFile::exists(last_filename))
		{
			std::cout << "Removing old configuration file: " << last_filename.toStdString() << "...";
			if(QFile::remove(last_filename))
				std::cout << "OK" << std::endl;
			else
				std::cout << "FAIL" << std::endl;
		}

		filename = QString::fromStdString(partition_mountpoint_prefix);
		filename.append(QString::fromUtf8("p%1-u/").arg(OS_PARTITION_NUMBER)).append(NET_SW_SETTINGS_PREFIX).append(ifaces[i]->name);
		
		QFile file(filename);

		if(ifaces[i]->enabled && file.open(QIODevice::WriteOnly))
		{
			/*
			 * DEVICE=eth0
			 * HWADDR=08:00:27:C9:2D:87
			 * IPADDR=208.164.186.1
			 * NETMASK=255.255.255.0
			 * ONBOOT=yes
			 * BOOTPROTO=none
			 */

			file.write("# This file was automatically generated by the " PROGRAM_NAME " program\n\n");
			file.write("DEVICE="); file.write(ifaces[i]->name.toStdString().c_str()); file.write("\n");
			file.write("HWADDR="); file.write(ifaces[i]->mac.toStdString().c_str()); file.write("\n");
// 			file.write("TYPE=Ethernet\n");
#ifdef CONFIGURABLE_IP
			if(ifaces[i]->ip.length() > 0 && ifaces[i]->subnetMask.length() > 0)
			{
				file.write("IPADDR="); file.write(ifaces[i]->ip.toStdString().c_str()); file.write("\n");
				file.write("NETMASK="); file.write(ifaces[i]->subnetMask.toStdString().c_str()); file.write("\n");
				file.write("BOOTPROTO=static\n");
			}
			else
#endif
				file.write("BOOTPROTO=none\n");				
// 			file.write("UUID=0a52f5f1-00ee-4239-b86c-fdd2ef7b0d41\n"); //this field is only used by network manager
			file.write("ONBOOT=yes\n");
			file.write("NM_CONTROLLED=no\n");
			ifaces[i]->name = ifaces[i]->last_valid_name;
		}
		file.close();
	}
	umountVpartition(OS_PARTITION_NUMBER);

	if(!machine->saveSettings())
	{
		std::cout << "saveSettings(): false" << std::endl;
		succeeded = false;
	}

	if(!machine->unlockMachine())
		return false;

	return succeeded;
}

bool VirtualMachine::saveSettingsRunTime()
{
	bool succeeded = true;

	for(int i = 0; i < ifaces_size; i++)
	{
		//Tipo interfaccia
		if(!machine->setIfaceAttachmentTypeRunTime(i, ifaces[i]->attachmentType))
		{
			std::cout << "setIfaceAttachmentType(" << i << ", " << ifaces[i]->attachmentType << ")" << std::endl;
			succeeded = false;
		}

		//Parametro in base al tipo di interfaccia
		if(!machine->setAttachmentDataRunTime(i, ifaces[i]->attachmentData))
		{
			std::cout << "setAttachmentData(" << i << ", " << ifaces[i]->attachmentData.toStdString() << ")" << std::endl;
			succeeded = false;
		}
	}

	return succeeded;
}

bool VirtualMachine::loadSettings(QString filename)
{
	return true;
}

#ifdef CONFIGURABLE_IP
int VirtualMachine::setIface(int iface, QString name, QString mac, QString ip, QString subnetMask)
#else
int VirtualMachine::setIface(int iface, QString name, QString mac)
#endif
{
	return 0;
}

void VirtualMachine::populateIfaces()
{
	std::vector<nsCOMPtr<INetworkAdapter> > networkAdapter_vec = machine->getNetworkInterfaces();

	uint8_t old_ifaces_size = ifaces_size;
	ifaces_size = networkAdapter_vec.size();
	if(old_ifaces_size != ifaces_size || ifaces == NULL)
	{
		ifaces = (Iface **)realloc(ifaces, sizeof(Iface*) * ifaces_size);
		for(int i = 0; i < old_ifaces_size; i++)
			delete ifaces[i];
	}

	mountVpartition(OS_PARTITION_NUMBER, true);
	for(int i = 0; i < ifaces_size; i++)
	{
// 		Iface(enabled, mac, cableConnected, attachmentType, attachmentData, name, ip, subnetMask);
		ifaces[i] = new Iface(
			  machine->getIfaceEnabled(networkAdapter_vec.at(i))
			, machine->getIfaceMac(networkAdapter_vec.at(i))
			, machine->getIfaceCableConnected(networkAdapter_vec.at(i))
			, machine->getAttachmentType(networkAdapter_vec.at(i))
			, machine->getAttachmentData(networkAdapter_vec.at(i), machine->getAttachmentType(networkAdapter_vec.at(i)))
			, getIfaceName(i)
#ifdef CONFIGURABLE_IP
			, getIp(i)
			, getSubnetMask(i)
#endif
		);
	}
	umountVpartition(OS_PARTITION_NUMBER);

	while(networkAdapter_vec.size() > 0)
	{
		networkAdapter_vec.back() = nsnull;
		networkAdapter_vec.pop_back();
	}
}

bool VirtualMachine::setNetworkAdapterData(int iface, ifacekey_t key, void *value_ptr)
{
	switch(key)
	{
		case IFACE_ENABLED:
		{
			bool *enabled = (bool *)value_ptr;
			return machine->setIfaceEnabled(iface, *enabled);
		}
		case IFACE_MAC:
		{
			QString *mac = (QString *)value_ptr;
			return machine->setIfaceMac(iface, *mac);
		}
		case IFACE_CONNECTED:
		{
			bool *cableConnected = (bool *)value_ptr;
			uint32_t machineState = machine->getState();
			if(machineState == MachineState::Running || machineState == MachineState::Paused)
				return machine->setCableConnectedRunTime(iface, *cableConnected);
			break;
		}
		case IFACE_NAME:
		{
			QString *name = (QString *)value_ptr;
// 			return ifaces[iface]->setName(*name);
			break;
		}
#ifdef CONFIGURABLE_IP
		case IFACE_IP:
		{
			QString *ip = (QString *)value_ptr;
// 			return ifaces[iface]->setIp(*ip);
			break;
		}
		case IFACE_SUBNETMASK:
		{
			QString *subnetMask = (QString *)value_ptr;
// 			return ifaces[iface]->setSubnetMask(*subnetMask);
			break;
		}
#endif
		case IFACE_ATTACHMENT_TYPE:
		{
			uint32_t *attachmentType = (uint32_t *)value_ptr;
			return machine->setIfaceAttachmentType(iface, *attachmentType);
		}
		case IFACE_ATTACHMENT_DATA:
		{
			QString *attachmentData = (QString *)value_ptr;
			return machine->setAttachmentData(iface, *attachmentData);
		}
		default:
			break;
	}
	return false;
}

bool VirtualMachine::operator==(VirtualMachine *vm)
{
// 	if (machine->getUUID() != vm->machine->getUUID())
// 		return false;

	if (ifaces_size != vm->ifaces_size)
		return false;

	for (int i = 0; i < ifaces_size; i++)
		if (ifaces[i] != vm->ifaces[i])
			return false;

	return true;
}

void VirtualMachine::operator=(const VirtualMachine &vm)
{
	while(ifaces_size > 0)
		delete ifaces[ifaces_size-- - 1];

	for (int i = 0; i < vm.ifaces_size; i++)
		ifaces[i] = vm.ifaces[i]->copyIface();

	ifaces_size = vm.ifaces_size;
}

// std::istream &VirtualMachine::VirtualMachine::operator>>(std::istream &is, VirtualMachine &vm)
// {
// 	is >> vm.ifaces_vec;
// 	is >> vm.vBoxVmFile.toStdString();
// 	return is;
// }
// 
// std::ostream &VirtualMachine::VirtualMachine::operator<<(std::ostream &os, const VirtualMachine &vm)
// {
// 	os << vm.ifaces_vec << '\n';
// 	os << vm.vBoxVmFile.toStdString();
// 	return os;
// }
