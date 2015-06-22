#include "VirtualBoxBridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iconv.h>

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
: vboxbridge(vboxbridge), machine(machine), session(vboxbridge->newSession())
{

}

MachineBridge::~MachineBridge()
{
	machine->Release();
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

QString MachineBridge::getHardDiskFilePath()
{
	return QString::fromUtf8("");
}

std::vector<INetworkAdapter*> MachineBridge::getNetworkInterfaces()
{
	std::vector<INetworkAdapter*> ifaces_vec;

	uint32_t chipsetType, maxNetworkAdapters;
	nsCOMPtr<ISystemProperties> sysProp;
	INetworkAdapter* network_adptr;

	machine->GetChipsetType(&chipsetType);

	vboxbridge->virtualBox->GetSystemProperties(getter_AddRefs(sysProp));
	sysProp->GetMaxNetworkAdapters(chipsetType, &maxNetworkAdapters);

	int i;
	for(i = 0; i < maxNetworkAdapters; i++)
	{
		machine->GetNetworkAdapter(i, &network_adptr);
		ifaces_vec.push_back(network_adptr);
	}

	sysProp = nsnull;
	return ifaces_vec;
}

QString MachineBridge::getIfaceMac(INetworkAdapter *iface)
{
	nsXPIDLString mac;
	iface->GetMACAddress(getter_Copies(mac));
	return returnQStringValue(mac);
}

bool MachineBridge::setIfaceMac(INetworkAdapter *iface, QString qMac)
{
	return true;
// 	return ((uint32_t) iface->SetMACAddress(qMac.toStdString().c_str()));
}

bool MachineBridge::getIfaceEnabled(INetworkAdapter *iface)
{
	PRBool enabled;
	iface->GetEnabled(&enabled);
	return enabled;
}

bool MachineBridge::start()
{
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

	nsXPIDLString type; type.AssignWithConversion("", 0);
	nsXPIDLString environment; environment.AssignWithConversion("", 0);
	nsCOMPtr<IProgress> progress;

	nsresult rc = machine->LaunchVMProcess(session, type, environment, getter_AddRefs(progress));
	if(!NS_SUCCEEDED(rc))
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
		std::cout << "[" << getName().toStdString() << "] Cannot launch VM!" << std::endl;
		return false;
	}

	return true;
}

bool MachineBridge::stop()
{
	nsresult rc;

	nsCOMPtr<IConsole> console;
	rc = session->GetConsole(getter_AddRefs(console));
	if(NS_FAILED(rc))
		return false;

	nsCOMPtr<IProgress> progress;
	rc = console->PowerDown(getter_AddRefs(progress));
	if(NS_FAILED(rc))
		return false;

	return true;
}
