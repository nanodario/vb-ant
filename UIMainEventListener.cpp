/*
 * VBANT - VirtualBox Advanced Network Tool
 * Copyright (C) 2015  Dario Messina
 * based on code by VirtualBox Open Source Edition (OSE) copyright (C) 2010-2013 Oracle Corporation
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
	connect(this, SIGNAL(sigNetworkAdapterChange(MachineBridge*,INetworkAdapter*)), parent(), SLOT(slotNetworkAdapterChange(MachineBridge*,INetworkAdapter*)));
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
	switch (aType)
	{
		/*
		 * All VirtualBox Events
		 */
		case VBoxEventType::OnStateChanged:
		{
			uint32_t machineState = 0;
			
			IStateChangedEvent *es;
			nsresult rc;
			QUERY_INTERFACE_AND_DEBUG_ERROR(IEvent, IStateChangedEvent, pEvent, es, rc);
			if(NS_SUCCEEDED(rc))
				es->GetState(&machineState);

			emit sigStateChange(machine, machineState);
			break;
		}

		case VBoxEventType::OnMachineStateChanged:
		{
			uint32_t machineState = 0;

			IMachineStateChangedEvent *es;
			nsresult rc;
			QUERY_INTERFACE_AND_DEBUG_ERROR(IEvent, IMachineStateChangedEvent, pEvent, es, rc);
			if(NS_SUCCEEDED(rc))
				es->GetState(&machineState);

			emit sigMachineStateChange(machine, machineState);
			break;
		}

		case VBoxEventType::OnSessionStateChanged:
		{
			uint32_t sessionState = 0;

			ISessionStateChangedEvent *es;
			nsresult rc;
			QUERY_INTERFACE_AND_DEBUG_ERROR(IEvent, ISessionStateChangedEvent, pEvent, es, rc);
			if(NS_SUCCEEDED(rc))
				es->GetState(&sessionState);

			emit sigSessionStateChange(machine, sessionState);
			break;
		}

		case VBoxEventType::OnNetworkAdapterChanged:
		{
			INetworkAdapter *nic;

			INetworkAdapterChangedEvent *es;
			nsresult rc;
			QUERY_INTERFACE_AND_DEBUG_ERROR(IEvent, INetworkAdapterChangedEvent, pEvent, es, rc);
			if(NS_SUCCEEDED(rc))
				es->GetNetworkAdapter(&nic);

			emit sigNetworkAdapterChange(machine, nic);
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

			emit sigRuntimeError(machine, fatal, returnQStringValue(strMessage));
			break;
		}

		default: break;
	}
	
	return NS_OK;
}

