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

VirtualBoxBridge::VirtualBoxBridge()
: virtualBox(nsnull)
{
	if(initXPCOM())
		initVirtualBox();
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
	rc = NS_GetMainEventQ(getter_AddRefs (nsCOM_eventQ));
	if (NS_FAILED(rc))
	{
		std::cerr << "Error: could not get main event queue! rc=" << rc << std::endl;
		return false;
	}

	return true;
}

bool VirtualBoxBridge::initVirtualBox()
{
	nsresult rc;

	/*
	 * Now XPCOM is ready and we can start to do real work.
	 * IVirtualBox is the root interface of VirtualBox and will be
	 * retrieved from the XPCOM component manager. We use the
	 * XPCOM provided smart pointer nsCOMPtr for all objects because
	 * that's very convenient and removes the need deal with reference
	 * counting and freeing.
	 */
	rc = NS_GetComponentManager(getter_AddRefs(nsCOM_manager));
	if (NS_FAILED(rc))
	{
		std::cerr << "Error: could not get component manager! rc=" << rc << std::endl;
		return false;
	}
	
	rc = nsCOM_manager->CreateInstanceByContractID(NS_VIRTUALBOX_CONTRACTID,
						 nsnull,
						 NS_GET_IID(IVirtualBox),
						 getter_AddRefs(virtualBox));
	if (NS_FAILED(rc))
	{
		std::cerr << "Error, could not instantiate VirtualBox object! rc" << rc << std::endl;
		return false;
	}
// 	std::cout << "VirtualBox object created :D" << std::endl;
	return true;
}

QString VirtualBoxBridge::getVBoxVersion()
{
	if(virtualBox != nsnull)
	{
		nsXPIDLString version;
		virtualBox->GetVersionNormalized(getter_Copies(version));
		return QString::fromAscii(ToNewCString(version));
	}
	return QString::fromUtf8("ERROR: Object virtualBox not initialized");
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
	return QString::QString::fromAscii(ToNewCString(new_mac));
}

std::vector<IMachine*> VirtualBoxBridge::getMachines()
{
	std::vector<IMachine*> machines_vec;
	if(virtualBox != nsnull)
	{
		IMachine **machines = NULL;
		uint32_t machineCnt = 0;

		if(NS_SUCCEEDED(virtualBox->GetMachines(&machineCnt, &machines)))
		{
			int i;
			for(i = 0; i < machineCnt; i++)
				machines_vec.push_back(machines[i]);
		}
	}
	return machines_vec;
}

QString MachineBridge::getName(IMachine *machine)
{
	nsXPIDLString name;
	machine->GetName(getter_Copies(name));
	return QString::fromAscii(ToNewCString(name));
}

std::vector<INetworkAdapter*> MachineBridge::getNetworkInterfaces(IVirtualBox *vbox, IMachine *machine)
{
	std::vector<INetworkAdapter*> ifaces_vec;

	uint32_t chipsetType, maxNetworkAdapters;
	nsCOMPtr<ISystemProperties> sysProp;
	INetworkAdapter* network_adptr;

	machine->GetChipsetType(&chipsetType);

	vbox->GetSystemProperties(getter_AddRefs(sysProp));
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
	return QString::fromAscii(ToNewCString(mac));
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
