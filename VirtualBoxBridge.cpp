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

#define VBOX_WITH_XPCOM

#include "VirtualBoxBridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <QPointer>
#include <QStringList>

/*
 * Include the XPCOM headers
 */
#include <nsMemory.h>
#include <nsString.h>
#include <nsIServiceManager.h>
#include <nsEventQueueUtils.h>
#include <nsIExceptionService.h>

/*
 * Include the VirtualBox headers
 */
#include <VBox/com/com.h>
#include <VBox/com/string.h>
#include <VBox/com/array.h>
#include <VBox/com/Guid.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>
#include <VBox/com/VirtualBox.h>
#include <VBox/com/listeners.h>

#include <iprt/stream.h>

/*
 * VirtualBox XPCOM interface. This header is generated
 * from IDL which in turn is generated from a custom XML format.
 */
#include "VirtualBox_XPCOM.h"
#include "Iface.h"
#include <QVector>

/*
 * HACK This code interacts with VirtualBox API every 500 ms. This is needed
 * because the event generation freezes the VM process, so there is a thread
 * which interacts with API to unlock situation.
 */
static void *knockAPI(void *_tparam)
{
	tparam_t *tparam = (tparam_t *)_tparam;
	nsXPIDLString vboxVersion;
	nsCOMPtr<IVirtualBox> vbox = tparam->virtualBox;
	
	while(1)
	{
		if(vbox != nsnull)
			vbox->GetVersion(getter_Copies(vboxVersion));
		else
			break;

// 		std::cout << "Knock knock API, are you still here?" << std::endl;
		usleep(500000);
	}
	
	std::cout << "IVirtualBox object destructed, stopping knocking ;)" << std::endl;
	pthread_exit(0);
}

VirtualBoxBridge::VirtualBoxBridge()
: virtualBox(nsnull)
{
	if(initXPCOM())
	{
		if(initVirtualBox())
		{
			std::cout << "VirtualBox object created" << std::endl;

			nsXPIDLString vboxVersion;
			nsXPIDLString vboxVersionNormalized;
			virtualBox->GetVersion(getter_Copies(vboxVersion));
			virtualBox->GetVersionNormalized(getter_Copies(vboxVersionNormalized));
			std::cout << "VirtualBox version:     " << returnQStringValue(vboxVersion).toStdString() << " (" << returnQStringValue(vboxVersionNormalized).toStdString() << ")" << std::endl;

			nsXPIDLString apiVersion;
			virtualBox->GetAPIVersion(getter_Copies(apiVersion));
			std::cout << "VirtualBox API version: " << returnQStringValue(apiVersion).toStdString() << std::endl;
			
			startAPIknocking();
		}
	}
}

VirtualBoxBridge::~VirtualBoxBridge()
{
	/* this is enough to free the IVirtualBox instance -- smart pointers rule! */
	virtualBox = nsnull;

	/*
	 * Process events that might have queued up in the XPCOM event
	 * queue. If we don't process them, the server might hang.
	 */
	nsCOM_eventQ->ProcessPendingEvents();
	
	nsCOM_manager = nsnull;
	nsCOM_eventQ = nsnull;
	nsCOM_serviceManager = nsnull;

	/*
	* Perform the standard XPCOM shutdown procedure.
	*/
	NS_ShutdownXPCOM(nsnull);
}

bool VirtualBoxBridge::initXPCOM()
{
	nsresult rc;

	/*
	 * This is the standard XPCOM init procedure.
	 * What we do is just follow the required steps to get an instance
	 * of our main interface, which is IVirtualBox.
	 *
	 * Note that we scope all nsCOMPtr variables in order to have all XPCOM
	 * objects automatically released before we call NS_ShutdownXPCOM at the
	 * end. This is an XPCOM requirement.
	 */
	rc = NS_InitXPCOM2(getter_AddRefs(nsCOM_serviceManager), nsnull, nsnull);
	if (NS_FAILED(rc))
	{
		std::cerr << "Error: XPCOM could not be initialized! rc=0x" << std::hex << rc << std::dec << std::endl;
		return false;
	}

	/*
	 * Make sure the main event queue is created. This event queue is
	 * responsible for dispatching incoming XPCOM IPC messages. The main
	 * thread should run this event queue's loop during lengthy non-XPCOM
	 * operations to ensure messages from the VirtualBox server and other
	 * XPCOM IPC clients are processed. This use case doesn't perform such
	 * operations so it doesn't run the event loop.
	 */
	rc = NS_GetMainEventQ(getter_AddRefs(nsCOM_eventQ));
	if (NS_FAILED(rc))
	{
		std::cerr << "Error: could not get main event queue! rc=0x" << std::hex << rc << std::dec << std::endl;
		return false;
	}

	/*
	 * Now XPCOM is ready and we can start to do real work.
	 * All interfaces will be retrieved from the XPCOM component manager.
	 * We use the XPCOM provided smart pointer nsCOMPtr for all objects
	 * because that's very convenient and removes the need deal with
	 * reference counting and freeing.
	 */
	rc = NS_GetComponentManager(getter_AddRefs(nsCOM_manager));
	if (NS_FAILED(rc))
	{
		std::cerr << "Error: could not get component manager! rc=0x" << std::hex << rc << std::dec << std::endl;
		return false;
	}

	return true;
}

bool VirtualBoxBridge::initVirtualBox()
{
	nsresult rc;

	/*
	 * IVirtualBox is the root interface of VirtualBox and will be
	 * retrieved from the XPCOM component manager.
	 */
	rc = nsCOM_manager->CreateInstanceByContractID(NS_VIRTUALBOX_CONTRACTID,
						 nsnull,
						 NS_GET_IID(IVirtualBox),
						 getter_AddRefs(virtualBox));
	
	if (NS_FAILED(rc))
	{
		std::cerr << "Error, could not instantiate VirtualBox object! rc=0x" << std::hex << rc << std::dec << std::endl;
		return false;
	}
	
	return true;
}

void VirtualBoxBridge::startAPIknocking()
{
	if(virtualBox == nsnull)
		return;

	tparam.virtualBox = virtualBox;
	pthread_create(&knockThread, NULL, knockAPI, &tparam);
}


nsCOMPtr<ISession> VirtualBoxBridge::newSession()
{
	nsresult rc;
	nsCOMPtr<ISession> s;

	/*
	 * The ISession interface represents a client process and allows
	 * for locking virtual machines to prevent conflicting changes
	 * to the machine. ISession interface will be retrieved from
	 * the XPCOM component manager.
	 */
	
	NS_CHECK_AND_DEBUG_ERROR(nsCOM_manager,
				 CreateInstanceByContractID(NS_SESSION_CONTRACTID,
							    nsnull,
							    NS_GET_IID(ISession),
							    getter_AddRefs(s)),
				 rc);

	if (NS_FAILED(rc))
	{
// 		std::cerr << "Error, could not instantiate Session object! rc=0x" << std::hex << rc << std::dec << std::endl;
		return nsnull;
	}
// 	else
// 		std::cout << "New Session object created" << std::endl;

	return s;
}

nsCOMPtr<IHost> VirtualBoxBridge::getHost()
{
	nsresult rc;
	nsCOMPtr<IHost> host;

	NS_CHECK_AND_DEBUG_ERROR(virtualBox, GetHost(getter_AddRefs(host)), rc);
	
	if(NS_SUCCEEDED(rc))
		return host;

	return nsnull;
}

std::vector<nsCOMPtr<IHostNetworkInterface> > VirtualBoxBridge::getHostNetworkInterfaces()
{
	std::vector<nsCOMPtr<IHostNetworkInterface> > host_ifaces_vec;

	uint32_t host_ifaces_size;
	IHostNetworkInterface **host_ifaces;

	getHost()->GetNetworkInterfaces(&host_ifaces_size, &host_ifaces);

	int i;
	for(i = 0; i < host_ifaces_size; i++)
	{
		uint32_t iface_type;
		nsXPIDLString iface_name;

		host_ifaces[i]->GetInterfaceType(&iface_type);
		host_ifaces[i]->GetName(getter_Copies(iface_name));
		if(iface_type == HostNetworkInterfaceType::Bridged)
		{
			bool new_iface = true;
			for(int j = 0; j < i; j++)
			{
				nsXPIDLString name;
				host_ifaces[j]->GetName(getter_Copies(name));
				if(returnQStringValue(name) == returnQStringValue(iface_name))
					new_iface = false;
			}
			if(new_iface)
				host_ifaces_vec.push_back(host_ifaces[i]);
		}
	}

	return host_ifaces_vec;
}

std::vector<QString> VirtualBoxBridge::getInternalNetworkList()
{
	std::vector<QString> internalNetworks_vec;
	short unsigned int **internalNetworks;
	uint32_t internalNetworks_size;
	
	virtualBox->GetInternalNetworks(&internalNetworks_size, &internalNetworks);
	
	for(int i = 0; i < internalNetworks_size; i++)
	{
		int j;
		for(j = 0; internalNetworks[i][j] != '\0'; j++);
		char *str = (char *)malloc(sizeof(char) * (j + 1));

		for(j = 0; internalNetworks[i][j] != '\0'; j++)
			str[j] = internalNetworks[i][j];
		str[j] = '\0';

		QString qstr = QString::fromUtf8(str);
		internalNetworks_vec.push_back(qstr);
		
		free(str);
	}
	return internalNetworks_vec;
}

std::vector<nsCOMPtr<INATNetwork> > VirtualBoxBridge::getNatNetworks()
{
	std::vector<nsCOMPtr<INATNetwork> > natNetworks_vec;
	uint32_t natNetworks_size;
	INATNetwork **natNetworks;
	
	virtualBox->GetNATNetworks(&natNetworks_size, &natNetworks);
	
	for (int i = 0; i < natNetworks_size; ++i)
		natNetworks_vec.push_back(natNetworks[i]);

	return natNetworks_vec;
}

#if 0
void UIMachineSettingsNetworkPage::refreshInternalNetworkList(bool fFullRefresh /* = false */)
{
/* Reload internal network list: */
m_internalNetworkList.clear();
/* Get internal network names from other VMs: */
if (fFullRefresh)
	m_internalNetworkList << otherInternalNetworkList();
/* Append internal network list with names from all the tabs: */
for (int iTab = 0; iTab < m_pTwAdapters->count(); ++iTab)
{
UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTwAdapters->widget(iTab));
if (pTab)
{
QString strName = pTab->alternativeName(KNetworkAttachmentType_Internal);
if (!strName.isEmpty() && !m_internalNetworkList.contains(strName))
	m_internalNetworkList << strName;
}
}
}

void UIMachineSettingsNetworkPage::refreshHostInterfaceList()
{
/* Reload host-only interface list: */
m_hostInterfaceList.clear();
const CHostNetworkInterfaceVector &ifaces = vboxGlobal().host().GetNetworkInterfaces();
for (int i = 0; i < ifaces.size(); ++i)
{
const CHostNetworkInterface &iface = ifaces[i];
if (iface.GetInterfaceType() == KHostNetworkInterfaceType_HostOnly && !m_hostInterfaceList.contains(iface.GetName()))
	m_hostInterfaceList << iface.GetName();
}
}

void UIMachineSettingsNetworkPage::refreshGenericDriverList(bool fFullRefresh /* = false */)
{
/* Load generic driver list: */
m_genericDriverList.clear();
/* Get generic driver names from other VMs: */
if (fFullRefresh)
	m_genericDriverList << otherGenericDriverList();
/* Append generic driver list with names from all the tabs: */
for (int iTab = 0; iTab < m_pTwAdapters->count(); ++iTab)
{
UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTwAdapters->widget(iTab));
if (pTab)
{
QString strName = pTab->alternativeName(KNetworkAttachmentType_Generic);
if (!strName.isEmpty() && !m_genericDriverList.contains(strName))
	m_genericDriverList << strName;
}
}
}

void UIMachineSettingsNetworkPage::refreshNATNetworkList()
{
/* Reload NAT network list: */
m_natNetworkList.clear();
const CNATNetworkVector &nws = vboxGlobal().virtualBox().GetNATNetworks();
for (int i = 0; i < nws.size(); ++i)
{
const CNATNetwork &nw = nws[i];
m_natNetworkList << nw.GetNetworkName();
}
}

/* static */
QStringList UIMachineSettingsNetworkPage::otherInternalNetworkList()
{
/* Load total internal network list of all VMs: */
CVirtualBox vbox = vboxGlobal().virtualBox();
QStringList otherInternalNetworks(QList<QString>::fromVector(vbox.GetInternalNetworks()));
return otherInternalNetworks;
}

/* static */
QStringList UIMachineSettingsNetworkPage::otherGenericDriverList()
{
/* Load total generic driver list of all VMs: */
CVirtualBox vbox = vboxGlobal().virtualBox();
QStringList otherGenericDrivers(QList<QString>::fromVector(vbox.GetGenericNetworkDrivers()));
return otherGenericDrivers;
}
#endif

QString VirtualBoxBridge::getVBoxVersion()
{
	QString retVal = QString::fromUtf8("ERROR: Object virtualBox not initialized");
	if(virtualBox != nsnull)
	{
		nsXPIDLString version;
		virtualBox->GetVersionNormalized(getter_Copies(version));
		retVal = returnQStringValue(version);
	}

	return retVal;
}

QString VirtualBoxBridge::generateMac()
{
	nsCOMPtr<IHost> host = nsnull;
	nsXPIDLString new_mac;

	if(virtualBox != nsnull)
	{
		virtualBox->GetHost(getter_AddRefs(host));

		if(host != nsnull)
			host->GenerateMACAddress(getter_Copies(new_mac));
	}

	host = nsnull;
	return returnQStringValue(new_mac);
}

std::vector<MachineBridge*> VirtualBoxBridge::getMachines(QObject *parent)
{
	std::vector<MachineBridge*> machines_vec;
	if(virtualBox != nsnull)
	{
		IMachine **machines = NULL;
		uint32_t machineCnt = 0;

		nsresult rc = virtualBox->GetMachines(&machineCnt, &machines);
		if(NS_SUCCEEDED(rc))
		{
			int i;
			for(i = 0; i < machineCnt; i++)
				machines_vec.push_back(new MachineBridge(this, machines[i], parent));
		}
	}
	return machines_vec;
}

MachineBridge::MachineBridge(VirtualBoxBridge *vboxbridge, IMachine *machine, QObject *parent)
: vboxbridge(vboxbridge), machine(machine), session(nsnull)
{
	eventListener.createObject();
	eventListener->init(new UIMainEventListener(this), parent);

	machine->GetId(getter_Copies(machineUUID));
}

MachineBridge::~MachineBridge()
{
	if(session != nsnull)
		session->UnlockMachine();
	while(machine->Release() > 0);
}

uint32_t MachineBridge::getMaxNetworkAdapters()
{
	uint32_t chipsetType, maxNetworkAdapters;
	nsCOMPtr<ISystemProperties> sysProp;
	
	machine->GetChipsetType(&chipsetType);
	vboxbridge->virtualBox->GetSystemProperties(getter_AddRefs(sysProp));
	sysProp->GetMaxNetworkAdapters(chipsetType, &maxNetworkAdapters);
	sysProp = nsnull;
	
	return maxNetworkAdapters;
}

QString MachineBridge::getUUID()
{
	nsXPIDLString iid;
	machine->GetId(getter_Copies(iid));
	return returnQStringValue(iid);
}

QString MachineBridge::getName()
{
	nsXPIDLString name;
	machine->GetName(getter_Copies(name));
	return returnQStringValue(name);
}

uint32_t MachineBridge::getState()
{
	if(session != nsnull)
	{
		uint32_t machineState;
		nsCOMPtr<IConsole> console;

		nsresult rc = session->GetConsole(getter_AddRefs(console));
		if(NS_FAILED(rc))
		{
			console = nsnull;
			return MachineState::Null;
		}

		rc = console->GetState(&machineState);
		console = nsnull;

		if(NS_SUCCEEDED(rc))
			return machineState;
	}

	return MachineState::Null;
}

uint32_t MachineBridge::getSessionState()
{
	if(session != nsnull)
	{
		uint32_t sessionState;

		nsresult rc = session->GetState(&sessionState);
		if(NS_SUCCEEDED(rc))
			return sessionState;
	}
	
	return SessionState::Null;
}

QString MachineBridge::getHardDiskFilePath()
{
	return QString::fromUtf8("");
}

std::vector<nsCOMPtr<INetworkAdapter> > MachineBridge::getNetworkInterfaces()
{
	std::vector<nsCOMPtr<INetworkAdapter> > ifaces_vec;

	uint32_t maxNetworkAdapters = getMaxNetworkAdapters();
	nsCOMPtr<INetworkAdapter> network_adptr;

	int i;
	for(i = 0; i < maxNetworkAdapters; i++)
	{
		machine->GetNetworkAdapter(i, getter_AddRefs(network_adptr));
		ifaces_vec.push_back(network_adptr);
	}

	return ifaces_vec;
}

QString MachineBridge::getIfaceMac(INetworkAdapter *iface)
{
	nsXPIDLString mac;
	iface->GetMACAddress(getter_Copies(mac));
	return returnQStringValue(mac);
}

bool MachineBridge::getIfaceCableConnected(INetworkAdapter *iface)
{
	PRBool cableConnected;
	iface->GetCableConnected(&cableConnected);
	return cableConnected;
}

uint32_t MachineBridge::getAttachmentType(INetworkAdapter *iface)
{
	nsresult rc;
	uint32_t attachmentType;
	
	NS_CHECK_AND_DEBUG_ERROR(iface, GetAttachmentType(&attachmentType), rc);
	
// 	PRINT_NETWORK_ATTACHMENT_TYPE(attachmentType, "");

	if(NS_FAILED(rc))
		return NetworkAttachmentType::Null;

	return attachmentType;
}

QString MachineBridge::getSubnetName(INetworkAdapter *iface)
{
	nsresult rc;
	nsXPIDLString subnetName;
	
	NS_CHECK_AND_DEBUG_ERROR(iface, GetInternalNetwork(getter_Copies(subnetName)), rc);
	
	return returnQStringValue(subnetName);
}

QString MachineBridge::getBridgedIface(INetworkAdapter *iface) //TODO
{
// 	ComPtr<IHost> host;
// 	
// 	vboxbridge->virtualBox->GetHost(host.asOutParam());
// 	host->GetNetworkInterfaces();

	nsresult rc;
	nsXPIDLString bridgedIface;
		
	NS_CHECK_AND_DEBUG_ERROR(iface, GetBridgedInterface(getter_Copies(bridgedIface)), rc);
	
	return returnQStringValue(bridgedIface);
}

bool MachineBridge::setIfaceEnabled(uint32_t iface, bool enabled)
{
	return setIfaceEnabled(getIface(iface), enabled);
}

bool MachineBridge::setIfaceMac(uint32_t iface, QString qMac)
{
	return  setIfaceMac(getIface(iface), qMac);
}

bool MachineBridge::setIfaceAttachmentType(uint32_t iface, uint32_t attachmentType)
{
	return setIfaceAttachmentType(getIface(iface), attachmentType);
}

bool MachineBridge::setCableConnected(uint32_t iface, bool connected)
{
	return setCableConnected(getIface(iface), connected);
}

bool MachineBridge::setSubnetName(uint32_t iface, QString qSubnetName)
{
	return setSubnetName(getIface(iface), qSubnetName);
}

bool MachineBridge::setBridgedIface(uint32_t iface, QString bridgedIface)
{
	return setBridgedIface(getIface(iface), bridgedIface);
}

ComPtr<INetworkAdapter> MachineBridge::getIface(uint32_t iface)
{
	ComPtr<INetworkAdapter> network_adptr;
	nsresult rc;
	NS_CHECK_AND_DEBUG_ERROR(machine, GetNetworkAdapter(iface, network_adptr.asOutParam()), rc);
	
	if(NS_FAILED(rc))
		network_adptr = nsnull;

	return network_adptr;
}

bool MachineBridge::setIfaceEnabled(ComPtr<INetworkAdapter> iface, bool enabled)
{
	nsresult rc;
	NS_CHECK_AND_DEBUG_ERROR(iface, SetEnabled(enabled), rc);
	
	return NS_SUCCEEDED(rc);
}

bool MachineBridge::setIfaceMac(ComPtr<INetworkAdapter> iface, QString qMac)
{
	QStringList qMacList = qMac.split(":");
	if (qMacList.size() != 6)
		return false;

	QString qMacUnformatted = QString(); 
	for (int i = 0; i < qMacList.size(); i++)
		qMacUnformatted += qMacList.at(i);

	nsXPIDLString mac; mac.AssignWithConversion(qMacUnformatted.toStdString().c_str());

	nsresult rc;
	NS_CHECK_AND_DEBUG_ERROR(iface, SetMACAddress(mac), rc);

	return NS_SUCCEEDED(rc);
}

bool MachineBridge::setIfaceAttachmentType(ComPtr<INetworkAdapter> iface, uint32_t attachmentType)
{
	nsresult rc;
	NS_CHECK_AND_DEBUG_ERROR(iface, SetAttachmentType(attachmentType), rc);

	return NS_SUCCEEDED(rc);
}

bool MachineBridge::setCableConnected(ComPtr<INetworkAdapter> iface, bool connected)
{
	if(iface == nsnull)
		return false;

	nsresult rc;
	NS_CHECK_AND_DEBUG_ERROR(iface, SetCableConnected(connected), rc);
	
	return NS_SUCCEEDED(rc);
}

bool MachineBridge::setCableConnectedRunTime(uint32_t iface, bool connected)
{
	nsresult rc;
	ComPtr<IMachine> tmp_machine;
	ComPtr<INetworkAdapter> nic;

	NS_CHECK_AND_DEBUG_ERROR(session, GetMachine(tmp_machine.asOutParam()), rc);
// 	NS_CHECK_AND_DEBUG_ERROR(tmp_machine, LockMachine(session, LockType::Write), rc);
	if(NS_SUCCEEDED(rc))
		NS_CHECK_AND_DEBUG_ERROR(tmp_machine, GetNetworkAdapter(iface, nic.asOutParam()), rc);
	
	bool done = setCableConnected(nic, connected);
// 	NS_CHECK_AND_DEBUG_ERROR(session, UnlockMachine(), rc);

	return done && NS_SUCCEEDED(rc);
}

bool MachineBridge::setSubnetName(ComPtr<INetworkAdapter> iface, QString qSubnetName)
{
	nsXPIDLString subnetName; subnetName.AssignWithConversion(qSubnetName.toStdString().c_str());

	nsresult rc;
	NS_CHECK_AND_DEBUG_ERROR(iface, SetInternalNetwork(subnetName), rc);
	
	return NS_SUCCEEDED(rc);
}

bool MachineBridge::setBridgedIface(ComPtr<INetworkAdapter> iface, QString qBridgedIface)
{
	nsXPIDLString bridgedIface; bridgedIface.AssignWithConversion(qBridgedIface.toStdString().c_str());
	
	nsresult rc;
	NS_CHECK_AND_DEBUG_ERROR(iface, SetBridgedInterface(bridgedIface), rc);
	
	return NS_SUCCEEDED(rc);
}

bool MachineBridge::getIfaceEnabled(INetworkAdapter *iface)
{
	PRBool enabled;
	iface->GetEnabled(&enabled);
	return enabled;
}

bool MachineBridge::start()
{
	nsresult rc;
	bool launchSucceeded;
	uint32_t sessionState, machineState;
	
	/*
	 * Session checking: check session for first launch or if it is still running
	 */
	if(session != nsnull)
	{
		machineState = getState();
		if(machineState == MachineState::Starting
			|| machineState == MachineState::Running
			|| machineState == MachineState::Paused)
		{
			std::cout << "[" << getName().toStdString() << "] VM is starting/running/paused" << std::endl;
			return false;
		}

		// Try to unlock session, then check if this action succeeded
		uint32_t state;
		NS_CHECK_AND_DEBUG_ERROR(session, UnlockMachine(), rc); //FIXME

		nsresult rc_tmp;
		NS_CHECK_AND_DEBUG_ERROR(session, GetState(&state), rc_tmp);

		// If Session is not unlocked, VM is still running
		if(NS_FAILED(rc) || state != SessionState::Unlocked)
		{
			std::cout << "[" << getName().toStdString() << "] Session locked!" << std::endl;
			return false;
		}
	}

	// Create new session
	session = vboxbridge->newSession();

	/*
	 * Launch routine: launch machine and check if it is launched or in starting state
	 */
	nsXPIDLString type; type.AssignWithConversion("", 0);
	nsXPIDLString environment; environment.AssignWithConversion("", 0);
	nsCOMPtr<IProgress> progress;

/*
	NS_CHECK_AND_DEBUG_ERROR(machine, LockMachine(session, LockType::Shared), rc);
	uint32_t state;

	nsresult rc_tmp;
	NS_CHECK_AND_DEBUG_ERROR(session, GetState(&state), rc_tmp);

	if(NS_FAILED(rc) || state != SessionState::Locked)
	{
		std::cout << "[" << getName().toStdString() << "] Can't lock Session!" << std::endl;
		return false;
	}
*/
	NS_CHECK_AND_DEBUG_ERROR(machine, LaunchVMProcess(session, type, environment, getter_AddRefs(progress)), rc);
	if(NS_FAILED(rc))
	{
		launchSucceeded = false;
		std::cout << "[" << getName().toStdString() << "] Cannot launch VM!" << std::endl;
		progress = nsnull;
	}
	else
	{
		launchSucceeded = true;
		PRInt32 resultCode;
		progress->WaitForCompletion(-1);
		progress->GetResultCode(&resultCode);

		if (resultCode != 0) // check success
		{
			std::cout << "[" << getName().toStdString() << "] Cannot launch VM! Result code: 0x" << std::hex << resultCode << std::dec << std::endl;
			return false;
		}
		registerListener();
	}	

	return launchSucceeded;
}

bool MachineBridge::stop(bool force)
{
	nsresult rc;
	
	if(console == nsnull)
	{
		rc = session->GetConsole(getter_AddRefs(console));
		if(NS_FAILED(rc))
		{
			console = nsnull;
			return false;
		}
	}

	nsCOMPtr<IProgress> progress;
	if(force)
		rc = console->PowerDown(getter_AddRefs(progress));
	else
		rc = console->PowerButton();

	progress = nsnull;

	if(NS_FAILED(rc))
		return false;

	session = nsnull;
	return true;
}

bool MachineBridge::pause(bool pauseEnabled)
{
	nsresult rc;

	if(console == nsnull)
	{
		rc = session->GetConsole(getter_AddRefs(console));
		if(NS_FAILED(rc))
		{
			console = nsnull;
			return false;
		}
	}
	
	if(pauseEnabled)
		rc = console->Pause();
	else
		rc = console->Resume();
	
	return NS_SUCCEEDED(rc);
}

bool MachineBridge::reset()
{
	nsresult rc;
	
	if(console == nsnull)
	{
		rc = session->GetConsole(getter_AddRefs(console));
		if(NS_FAILED(rc))
		{
			console = nsnull;
			return false;
		}
	}
	
	rc = console->Reset();
	
	return NS_SUCCEEDED(rc);
}

bool MachineBridge::openSettings()
{
// 	/* Create VM settings window on the heap!
// 	 * Its necessary to allow QObject hierarchy cleanup to delete this dialog if necessary: */
// 	QPointer<UISettingsDialogMachine> pDialog = new UISettingsDialogMachine(NULL, //activeMachineWindow(),
// 										getUUID(), //session->GetMachine().GetMachine().GetId(),
// 										QString(), QString());
// 
// 	/* Executing VM settings window.
// 	 * This blocking function calls for the internal event-loop to process all further events,
// 	 * including event which can delete the dialog itself. */
// 	pDialog->execute();
// 	/* Delete dialog if its still valid: */
// 	if (pDialog)
// 		delete pDialog;
// 
// 	return true;
	return false;
}

QString MachineBridge::getIfaceMac(int iface)
{
	return getIfaceMac(getIface(iface));
}

QString MachineBridge::getIfaceFormattedMac(int iface)
{
	return getIfaceFormattedMac(getIface(iface));
}

QString MachineBridge::getIfaceFormattedMac(INetworkAdapter *iface)
{
	std::string unformattedMac = getIfaceMac(iface).toStdString();
	std::string formattedMac("");
	
	formattedMac.append(unformattedMac.substr(0, 2)).append(":");
	formattedMac.append(unformattedMac.substr(2, 2)).append(":");
	formattedMac.append(unformattedMac.substr(4, 2)).append(":");
	formattedMac.append(unformattedMac.substr(6, 2)).append(":");
	formattedMac.append(unformattedMac.substr(8, 2)).append(":");
	formattedMac.append(unformattedMac.substr(10, 2));
	
	return QString::fromStdString(formattedMac);
}

bool MachineBridge::saveSettings()
{
	PRBool settingsModified;
	nsresult rc;

	NS_CHECK_AND_DEBUG_ERROR(machine, GetSettingsModified(&settingsModified), rc);
	
	if(NS_SUCCEEDED(rc) && settingsModified)
		NS_CHECK_AND_DEBUG_ERROR(machine, SaveSettings(), rc);
	else
		return NS_SUCCEEDED(rc);

	return NS_SUCCEEDED(rc);
}

bool MachineBridge::shutdownVMProcess()
{
	nsresult rc;

	if(session != nsnull)
	{
		uint32_t machineState = getState();
		if(machineState == MachineState::Starting
			|| machineState == MachineState::Running
			|| machineState == MachineState::Paused)
		{
			std::cout << "[" << getName().toStdString() << "] VM is starting/running/paused" << std::endl;
			return false;
		}

		// Try to unlock session, then check if this action succeeded
		uint32_t state;
		NS_CHECK_AND_DEBUG_ERROR(session, UnlockMachine(), rc); //FIXME

		nsresult rc_tmp;
		NS_CHECK_AND_DEBUG_ERROR(session, GetState(&state), rc_tmp);

		// If Session is not unlocked, VM is still running
		if(NS_FAILED(rc) || state != SessionState::Unlocked)
		{
			std::cout << "[" << getName().toStdString() << "] Session locked!" << std::endl;
			return false;
		}
	}

	session = nsnull;
	return true;
}

bool MachineBridge::registerListener() //TODO select only events that I'm interested to
{
	nsresult rc;

	if(session == nsnull)
		return false;
	
	NS_CHECK_AND_DEBUG_ERROR(session, GetConsole(getter_AddRefs(console)), rc);
	if(NS_FAILED(rc))
		return false;

	NS_CHECK_AND_DEBUG_ERROR(console, GetEventSource(getter_AddRefs(eventSource)), rc);
	if(NS_FAILED(rc))
		return false;
	
// 	QVector<VBoxEventType> events;
	uint32_t events[1];
// 	events << VBoxEventType::Any;
	events[0] = VBoxEventType::Any;
	NS_CHECK_AND_DEBUG_ERROR(eventSource, RegisterListener(eventListener, /*events.count()*/ 1, events, (PRBool) true), rc);
	if(NS_FAILED(rc))
		return false;

	return true;
}

bool MachineBridge::lockMachine()
{
// 	std::cout << "[" << getName().toStdString() << "] Trying to lock machine..." << std::endl;
	nsresult rc;
	uint32_t state;

	uint32_t machineState = getState();

	if(machineState == MachineState::Starting
		|| machineState == MachineState::Running
		|| machineState == MachineState::Paused)
		return false;

	if(session != nsnull)
	{
		// Try to unlock session, then check if this action succeeded
		NS_CHECK_AND_DEBUG_ERROR(session, UnlockMachine(), rc);
		
		nsresult rc_tmp;
		GET_AND_DEBUG_MACHINE_STATE(session, state, rc_tmp);
		
		// If Session is not unlocked, VM is still running
		
		if(NS_FAILED(rc) || state != SessionState::Unlocked)
		{
// 			std::cout << "[" << getName().toStdString() << "] Session already locked!" << std::endl;
			return false;
		}
	}

	session = vboxbridge->newSession();

	GET_AND_DEBUG_MACHINE_STATE(session, state, rc);

	if(state == SessionState::Unlocked)
		NS_CHECK_AND_DEBUG_ERROR(machine, LockMachine(session, LockType::Write), rc);

	GET_AND_DEBUG_MACHINE_STATE(session, state, rc);

	if(NS_SUCCEEDED(rc) && state == SessionState::Locked)
		NS_CHECK_AND_DEBUG_ERROR(session, GetMachine(&machine), rc);

// 	if(NS_SUCCEEDED(rc) && state == SessionState::Locked)
// 		std::cout << "[" << getName().toStdString() << "] Machine locked" << std::endl;
// 	else
// 		std::cout << "[" << getName().toStdString() << "] Failed to lock machine (rc: 0x" << std::hex << rc << std::dec << ")" << std::endl;

	return NS_SUCCEEDED(rc);
}

bool MachineBridge::unlockMachine()
{
// 	std::cout << "[" << getName().toStdString() << "] Trying to unlock machine..." << std::endl;

	nsresult rc;
	uint32_t state;
	GET_AND_DEBUG_MACHINE_STATE(session, state, rc);

	NS_CHECK_AND_DEBUG_ERROR(session, UnlockMachine(), rc);
	
// 	if(NS_SUCCEEDED(rc) && state == SessionState::Unlocked)
// 		std::cout << "[" << getName().toStdString() << "] Machine unlocked" << std::endl;
// 	else
// 		std::cout << "[" << getName().toStdString() << "] Failed to unlock machine (rc: 0x" << std::hex << rc << std::dec << ")" << std::endl;
	
	GET_AND_DEBUG_MACHINE_STATE(session, state, rc);
	
	//HACK FIXME Renew machine object because actual object is unlockable
	NS_CHECK_AND_DEBUG_ERROR(vboxbridge->virtualBox, FindMachine(machineUUID, &machine), rc);

	session = nsnull;

	return NS_SUCCEEDED(rc);
}
