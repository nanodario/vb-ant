/* $Id: UIMainEventListener.cpp $ */
/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * UIMainEventListener class implementation
 */

/*
 * Copyright (C) 2010-2013 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* GUI includes: */
#include "UIMainEventListener.h"
#include "VirtualBoxBridge.h"

#include <QObject>
#include <QMetaType>
#include <iostream>

/* COM includes: */

UIMainEventListener::UIMainEventListener(MachineBridge *machine) : QObject()
, machine(machine)
{
	/* For queued events we have to extra register our enums/interface classes
	 * (Q_DECLARE_METATYPE isn't sufficient).
	 * Todo: Try to move this to a global function, which is auto generated
	 * from xslt. */
// 	qRegisterMetaType<MachineState>("MachineState");
// 	qRegisterMetaType<SessionState>("SessionState");
// 	qRegisterMetaType<INetworkAdapter>("INetworkAdapter");
	qRegisterMetaType<uint32_t>("uint32_t");
// 	qRegisterMetaType<MachineBridge>("MachineBridge");
}

HRESULT UIMainEventListener::init(QObject *pParent)
{
	setParent(pParent);
	
	connect(this, SIGNAL(sigStateChange(MachineBridge*, uint32_t)), parent(), SLOT(slotStateChange(MachineBridge*, uint32_t)));
	return NS_OK;
}

void    UIMainEventListener::uninit()
{
}

/**
 * @todo: instead of double wrapping of events into signals maybe it
 * make sense to use passive listeners, and peek up events in main thread.
 */
STDMETHODIMP UIMainEventListener::HandleEvent(uint32_t aType, IEvent *pEvent)
{
// 	CEvent event(pEvent);
// 	printf("Event received: %d\n", event.GetType());
// 	uint32_t eventType;
// 	pEvent->GetType(&eventType);
	
	std::cout << "Evento: " << aType << std::endl;
	switch (/*event.GetType()*/ aType)
	{
		/*
		 * All VirtualBox Events
		 */
		case VBoxEventType::OnStateChanged:
		{
// 			nsXPIDLString machineID;
			uint32_t machineState = 0;
			
			IStateChangedEvent *es;
			nsresult rc;
			QUERY_INTERFACE_AND_DEBUG_ERROR(IEvent, IStateChangedEvent, pEvent, es, rc);
			if(NS_SUCCEEDED(rc))
			{
// 				es->GetMachineId(&machineID);
// 				std::cout << "machineID: " << machineID << std::endl;
				es->GetState(&machineState);
				std::cout << "machineState: " << machineState << std::endl;
			}
			
// 			emit sigMachineStateChange(pEvent->GetIID(), machineState);
			emit sigStateChange(machine, machineState);
			break;
		}

		case VBoxEventType::OnMachineStateChanged:
		{
// 			MachineStateChangedEvent es(pEvent);
// 			emit sigMachineStateChange(es.GetMachineId(), es.GetState());

			uint32_t machineID;
			uint32_t machineState = 0;

			IMachineStateChangedEvent *es;
			nsresult rc;
			QUERY_INTERFACE_AND_DEBUG_ERROR(IEvent, IMachineStateChangedEvent, pEvent, es, rc);
			if(NS_SUCCEEDED(rc))
			{
// 				es->GetMachineId(&machineID);
// 				std::cout << "machineID: " << machineID << std::endl;
				es->GetState(&machineState);
				std::cout << "machineState: " << machineState << std::endl;
			}
			
// 			emit sigMachineStateChange(pEvent->GetIID(), machineState);
			emit sigMachineStateChange(QString("strId"), machineState);
			break;
		}

		case VBoxEventType::OnSessionStateChanged:
		{
			uint32_t machineID;
			uint32_t sessionState;

			ISessionStateChangedEvent *es;
			nsresult rc;
			QUERY_INTERFACE_AND_DEBUG_ERROR(IEvent, ISessionStateChangedEvent, pEvent, es, rc);
			if(NS_SUCCEEDED(rc))
			{
// 				es->GetMachineId(&machineID);
// 				std::cout << "machineID: " << machineID << std::endl;
				es->GetState(&sessionState);
				std::cout << "machineState: " << sessionState << std::endl;
			}
						
			emit sigSessionStateChange(QString("strId"), sessionState);
			break;
		}

		case VBoxEventType::OnNetworkAdapterChanged:
		{
			INetworkAdapterChangedEvent *es;
			INetworkAdapter *nic;
			nsresult rc;
			QUERY_INTERFACE_AND_DEBUG_ERROR(IEvent, INetworkAdapterChangedEvent, pEvent, es, rc);
			if(NS_SUCCEEDED(rc))
				es->GetNetworkAdapter(&nic);

			emit sigNetworkAdapterChange(nic);
			break;
		}

		case VBoxEventType::OnRuntimeError:
		{
			PRBool fatal;
			nsXPIDLString strMessage;
			
			IRuntimeErrorEvent *es;
			nsresult rc;
			QUERY_INTERFACE_AND_DEBUG_ERROR(IEvent, IRuntimeErrorEvent, pEvent, es, rc);
			if(NS_SUCCEEDED(rc))
			{
				es->GetFatal(&fatal);
				es->GetMessage(getter_Copies(strMessage));
			}
			
			emit sigRuntimeError(fatal, QString("strId"), returnQStringValue(strMessage));
			break;
		}

		default: break;
	}
	
// 	machine->vboxbridge->knockAPI(); //HACK
	
	return NS_OK;
}

