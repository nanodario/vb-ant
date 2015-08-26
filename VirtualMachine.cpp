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

#include <sys/mount.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <malloc.h>

#include "VirtualBoxBridge.h"

VirtualMachine::VirtualMachine(MachineBridge *machine)
: machine(machine), ifaces_size(0)
{
	std::vector<nsCOMPtr<INetworkAdapter> > networkAdapter_vec = machine->getNetworkInterfaces();
	ifaces_size = networkAdapter_vec.size();
	ifaces = (Iface **)malloc(sizeof(Iface*) * ifaces_size);

	for(int i = 0; i < ifaces_size; i++)
	{
// 		Iface(enabled, mac, cableConnected, attachmentType, subnetName, name, ip, subnetMask);
		Iface *iface = new Iface(
			machine->getIfaceEnabled(networkAdapter_vec.at(i)),
			machine->getIfaceMac(networkAdapter_vec.at(i)),
			machine->getIfaceCableConnected(networkAdapter_vec.at(i)),
			machine->getAttachmentType(networkAdapter_vec.at(i)),
			machine->getSubnetName(networkAdapter_vec.at(i))
		);

		ifaces[i] = iface;
	}

	while(networkAdapter_vec.size() > 0)
	{
		networkAdapter_vec.back() = nsnull;
		networkAdapter_vec.pop_back();
	}
}

VirtualMachine::~VirtualMachine()
{
	while(ifaces_size > 0)
		delete ifaces[ifaces_size-- - 1];

	free(ifaces);
}

bool VirtualMachine::mountVM(bool readonly)
{
	return true;
}

bool VirtualMachine::umountVM()
{
	return true;
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
	
	i->enabled = machine->getIfaceEnabled(iface);
	i->setMac(machine->getIfaceMac(iface));
	i->cableConnected = machine->getIfaceCableConnected(iface);
	i->setAttachmentType(machine->getAttachmentType(iface));
	i->setSubnetName(machine->getSubnetName(iface));
}

QString VirtualMachine::getIp(uint32_t iface)
{
	return QString("10.10.10.%1").arg(iface);
}

QString VirtualMachine::getSubnetMask(uint32_t iface)
{
	return QString("%1").arg(iface);
}

bool VirtualMachine::saveSettings()
{
	bool succeeded = true;
	
	if(!machine->lockMachine())
	{
		std::cerr << "[" << machine->getName().toStdString() <<  "] Cannot lock machine" << std::endl;
		return false;
	}
	
	for(int i = 0; i < ifaces_size; i++)
	{
		//Abilita
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
			
			switch(ifaces[i]->attachmentType)
			{
				//Nome sottorete
				/* TODO il subnetname deve essere utilizzato solamente nel caso 
				* in cui il metodo di attach sia NetworkAttachmentType::Internal
				*/
				case NetworkAttachmentType::Internal:
				{
					if(!machine->setSubnetName(i, ifaces[i]->subnetName))
					{
						std::cout << "setSubnetName(" << i << ", " << ifaces[i]->subnetName.toStdString() << ")" << std::endl;
						succeeded = false;
					}
					break;
				}
				case NetworkAttachmentType::Bridged:
				{
					if(!machine->setBridgedIface(i, ifaces[i]->subnetName))
					{
						std::cout << "setSubnetName(" << i << ", " << ifaces[i]->subnetName.toStdString() << ")" << std::endl;
						succeeded = false;
					}
					break;
				}	
				case NetworkAttachmentType::Null:
// 				case NetworkAttachmentType::Bridged:
				case NetworkAttachmentType::Generic:
				case NetworkAttachmentType::HostOnly:
// 				case NetworkAttachmentType::Internal:
				case NetworkAttachmentType::NAT:
				case NetworkAttachmentType::NATNetwork:
					break;
				default:
					succeeded = false;
					break;
			}
		}
	}

	for(int i = 0; i < ifaces_size; i++)
	{
		/*
		 * TODO: scrivere sull'hard disk della VM la configurazione per le interfacce
		 * //Nome
		 *
		 * # PCI device 0x14e4:0x167d (tg3)
		 * SUBSYSTEM=="net", ACTION=="add", DRIVERS=="?*", ATTR{address}=="00:11:25:d3:6d:d1", ATTR{dev_id}=="0x0", ATTR{type}=="1", KERNEL=="eth*", NAME="eth0"
		 * 
		 */
		std::string line = "";

		line.append("# (");
		line.append(ifaces[i]->mac.toStdString());
		line.append(") ");
		line.append(ifaces[i]->name.toStdString());
		std::cout << line << std::endl;

		line.clear();
		line.append("SUBSYSTEM==\"net\", ACTION==\"add\", DRIVERS==\"?*\", ATTR{address}==\"");
		line.append(ifaces[i]->mac.toStdString());
		line.append("\", ATTR{dev_id}==\"0x0\", ATTR{type}==\"1\", KERNEL==\"eth*\", NAME=\"");
		line.append(ifaces[i]->name.toStdString());
		line.append("\"");
		std::cout << line << std::endl << std::endl;

#ifdef CONFIGURABLE_IP
		/*
		 * TODO: scrivere sull'hard disk della VM gli indirizzi ip delle interfacce
		 * //Indirizzo IP
		 * //Maschera sottorete
		 */
#endif
	}
	
	if(!machine->saveSettings())
	{
		std::cout << "saveSettings(): false" << std::endl;
		succeeded = false;
	}	
	if(!machine->unlockMachine())
		return false;

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
			return machine->setCableConnectedRunTime(iface, *cableConnected);
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
			return ifaces[iface]->setIp(*ip);
			break;
		}
		case IFACE_SUBNETMASK:
		{
			QString *subnetMask = (QString *)value_ptr;
			return ifaces[iface]->setSubnetMask(*subnetMask);
			break;
		}
#endif
		case IFACE_ATTACHMENT_TYPE:
		{
			uint32_t *attachmentType = (uint32_t *)value_ptr;
			return machine->setIfaceAttachmentType(iface, *attachmentType);
		}
		case IFACE_SUBNETNAME:
		{
			QString *subnetName = (QString *)value_ptr;
			return machine->setSubnetName(iface, *subnetName);
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