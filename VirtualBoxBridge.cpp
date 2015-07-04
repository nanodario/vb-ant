#define VBOX_WITH_XPCOM

#include "VirtualBoxBridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
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

#include <iprt/stream.h>

/*
 * VirtualBox XPCOM interface. This header is generated
 * from IDL which in turn is generated from a custom XML format.
 */
#include "VirtualBox_XPCOM.h"
#include "Iface.h"

QString returnQStringValue(nsXPIDLString s)
{
	const char *str = ToNewCString(s);
	QString retVal = QString::fromAscii(str);
	free((void*)str);
	return retVal;
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
			std::cout << "VirtualBox version:     " << returnQStringValue(vboxVersion).toStdString() << " (" << returnQStringValue(vboxVersion).toStdString() << ")" << std::endl;

			nsXPIDLString apiVersion;
			virtualBox->GetAPIVersion(getter_Copies(apiVersion));
			std::cout << "VirtualBox API version: " << returnQStringValue(apiVersion).toStdString() << std::endl;
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
		std::cerr << "Error: XPCOM could not be initialized! rc=" << rc << std::endl;
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
		std::cerr << "Error: could not get main event queue! rc=" << rc << std::endl;
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
		std::cerr << "Error: could not get component manager! rc=" << rc << std::endl;
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
		std::cerr << "Error, could not instantiate VirtualBox object! rc=" << rc << std::endl;
		return false;
	}
	return true;
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
	rc = nsCOM_manager->CreateInstanceByContractID(NS_SESSION_CONTRACTID,
						       nsnull,
						       NS_GET_IID(ISession),
						       getter_AddRefs(s));

	if (NS_FAILED(rc))
	{
		std::cerr << "Error, could not instantiate Session object! rc=" << rc << std::endl;
		return nsnull;
	}

	return s;
}

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

std::vector<MachineBridge*> VirtualBoxBridge::getMachines()
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
				machines_vec.push_back(new MachineBridge(this, machines[i]));
		}
	}
	return machines_vec;
}

MachineBridge::MachineBridge(VirtualBoxBridge *vboxbridge, IMachine *machine)
: vboxbridge(vboxbridge), machine(machine), session(nsnull)
{
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
	else
		return MachineState::Null;
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

bool MachineBridge::setIfaceMac(uint32_t iface, QString qMac)
{
	return  setIfaceMac(getIface(iface), qMac);
}

bool MachineBridge::enableIface(uint32_t iface, uint32_t attachmentType)
{
	return enableIface(getIface(iface), attachmentType);
}

bool MachineBridge::disableIface(uint32_t iface)
{
	return disableIface(getIface(iface));
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

bool MachineBridge::enableIface(ComPtr<INetworkAdapter> iface, uint32_t attachmentType)
{
	nsresult rc1, rc2, rc3;
	
	NS_CHECK_AND_DEBUG_ERROR(iface, SetEnabled(true), rc1);
	NS_CHECK_AND_DEBUG_ERROR(iface, SetAttachmentType(attachmentType), rc2);
	NS_CHECK_AND_DEBUG_ERROR(iface, SetCableConnected(true), rc3);
	
	return NS_SUCCEEDED(rc1) && NS_SUCCEEDED(rc2) && NS_SUCCEEDED(rc3);
}

bool MachineBridge::disableIface(ComPtr<INetworkAdapter> iface)
{
	nsresult rc1, rc2, rc3;
	
	NS_CHECK_AND_DEBUG_ERROR(iface, SetEnabled(false), rc1);
	NS_CHECK_AND_DEBUG_ERROR(iface, SetAttachmentType(NetworkAttachmentType::Null), rc2);
	NS_CHECK_AND_DEBUG_ERROR(iface, SetCableConnected(false), rc3);
	
	return NS_SUCCEEDED(rc1) && NS_SUCCEEDED(rc2) && NS_SUCCEEDED(rc3);
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
	uint32_t sessionState, machineState;
	
	/*
	 * Session checking: check session for first launch or if it is still running
	 */
	if(session != nsnull)
	{
		machineState = getState();
		if(machineState == MachineState::Starting || machineState == MachineState::Running)
		{
			std::cout << "[" << getName().toStdString() << "] VM is starting/running" << std::endl;
			return false;
		}

		// Try to unlock session, then check if this action succeeded
		session->UnlockMachine();
		
		uint32_t state;
		session->GetState(&state);
		
		// If Session is not unlocked, VM is still running
		if(state != SessionState::Unlocked)
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

	NS_CHECK_AND_DEBUG_ERROR(machine, LaunchVMProcess(session, type, environment, getter_AddRefs(progress)), rc);
	if(NS_FAILED(rc))
	{
		std::cout << "[" << getName().toStdString() << "] Cannot launch VM!" << std::endl;
		progress = nsnull;
		return false;
	}

	PRInt32 resultCode;
	progress->WaitForCompletion(-1);
	progress->GetResultCode(&resultCode);
	progress = nsnull;
	
	if (resultCode != 0) // check success
	{
		std::cout << "[" << getName().toStdString() << "] Cannot launch VM! Result code: 0x" << std::hex << resultCode << std::dec << std::endl;
		return false;
	}

	return true;
}

bool MachineBridge::stop(bool force)
{
	nsresult rc;

	nsCOMPtr<IConsole> console;
	rc = session->GetConsole(getter_AddRefs(console));
	if(NS_FAILED(rc))
	{
		console = nsnull;
		return false;
	}

	nsCOMPtr<IProgress> progress;
	if(force)
		rc = console->PowerDown(getter_AddRefs(progress));
	else
		rc = console->PowerButton();

	progress = nsnull;
	console = nsnull;
	if(NS_FAILED(rc))
		return false;

	session = nsnull;
	return true;
}

bool MachineBridge::pause()
{
	return false;
}

bool MachineBridge::reset()
{
	return false;
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

bool MachineBridge::lockMachine()
{
// 	std::cout << "[" << getName().toStdString() << "] Trying to lock machine..." << std::endl;
	nsresult rc;
	uint32_t state;

	if(session != nsnull)
	{
		// Try to unlock session, then check if this action succeeded
		session->UnlockMachine();
		
		GET_AND_DEBUG_MACHINE_STATE(session, state, rc);
		
		// If Session is not unlocked, VM is still running
		
		if(state != SessionState::Unlocked)
		{
// 			std::cout << "[" << getName().toStdString() << "] Session already locked!" << std::endl;
			return false;
		}
	}
	
	// Create new session
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
