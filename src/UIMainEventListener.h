/*
 * VB-ANT - VirtualBox - Advanced Network Tool
 * Copyright (C) 2015  Dario Messina
 * based on code by VirtualBox Open Source Edition (OSE) copyright (C) 2010-2013 Oracle Corporation
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

#ifndef UIMAINEVENTLISTENER_H
#define UIMAINEVENTLISTENER_H

#include <QObject>

#define RT_OS_LINUX
#define VBOX_WITH_XPCOM
#define IN_RING3

/* Other VBox includes: */
#include <VBox/com/listeners.h>
#include "VirtualBoxBridge.h"

#ifdef DEBUG_FLAG
	#define QUERY_INTERFACE_AND_DEBUG_ERROR(srcIface, dstIface, srcPtr, dstPtr, rc)	\
	do {										\
		rc = CallQueryInterface<srcIface, dstIface>(srcPtr, &dstPtr);		\
		if(NS_FAILED(rc))							\
		{									\
			std::cerr << "[" << __FILE__ << ":" << __LINE__ << " -> " << __func__ << "] " \
			<< "CallQueryInterface<"					\
			<< #srcIface << ", " << #dstIface				\
			<< ">(" << #srcPtr << ", &" << #dstPtr				\
			<< "): 0x" << std::hex << rc << std::dec << std::endl;		\
		}									\
	} while(0);
#else
	#define QUERY_INTERFACE_AND_DEBUG_ERROR(srcIface, dstIface, srcPtr, dstPtr, rc)	\
	do { rc = CallQueryInterface<srcIface, dstIface>(srcPtr, &dstPtr); } while(0);
#endif
/* Note: On a first look this may seems a little bit complicated.
 * There are two reasons to use a separate class here which handles the events
 * and forward them to the public class as signals. The first one is that on
 * some platforms (e.g. Win32) this events not arrive in the main GUI thread.
 * So there we have to make sure they are first delivered to the main GUI
 * thread and later executed there. The second reason is, that the initiator
 * method may hold a lock on a object which has to be manipulated in the event
 * consumer. Doing this without being asynchronous would lead to a dead lock. To
 * avoid both problems we send signals as a queued connection to the event
 * consumer. Qt will create a event for us, place it in the main GUI event
 * queue and deliver it later on. */

class MachineBridge;

class UIMainEventListener: public QObject
{
	Q_OBJECT;

	public:
		UIMainEventListener(MachineBridge *machine);

		HRESULT init(QObject *pParent);
		void    uninit();

		STDMETHOD(HandleEvent)(uint32_t aType, IEvent *pEvent);

	signals:
		/* All VirtualBox Signals */
		void sigMachineStateChange(MachineBridge *machine, uint32_t state);
//     void sigMachineDataChange(QString strId);
//     void sigExtraDataCanChange(QString strId, QString strKey, QString strValue, bool &fVeto, QString &strVetoReason); /* use Qt::DirectConnection */
//     void sigExtraDataChange(QString strId, QString strKey, QString strValue);
//     void sigMachineRegistered(QString strId, bool fRegistered);
		void sigSessionStateChange(MachineBridge *machine, uint32_t state);
//     void sigSnapshotTake(QString strId, QString strSnapshotId);
//     void sigSnapshotDelete(QString strId, QString strSnapshotId);
//     void sigSnapshotChange(QString strId, QString strSnapshotId);

		/* All Console Signals */
//     void sigMousePointerShapeChange(bool fVisible, bool fAlpha, QPoint hotCorner, QSize size, QVector<uint8_t> shape);
//     void sigMouseCapabilityChange(bool fSupportsAbsolute, bool fSupportsRelative, bool fSupportsMultiTouch, bool fNeedsHostCursor);
//     void sigKeyboardLedsChangeEvent(bool fNumLock, bool fCapsLock, bool fScrollLock);
		void sigStateChange(MachineBridge *machine, uint32_t state);
//     void sigAdditionsChange();
		void sigNetworkAdapterChange(MachineBridge *machine, INetworkAdapter *nic);
//     void sigMediumChange(MediumAttachment attachment);
//     void sigVRDEChange();
//     void sigVideoCaptureChange();
//     void sigUSBControllerChange();
//     void sigUSBDeviceStateChange(CUSBDevice device, bool fAttached, CVirtualBoxErrorInfo error);
//     void sigSharedFolderChange();
		void sigRuntimeError(MachineBridge *machine, bool fFatal, QString strMessage);
//     void sigCanShowWindow(bool &fVeto, QString &strReason); /* use Qt::DirectConnection */
//     void sigShowWindow(LONG64 &winId); /* use Qt::DirectConnection */
//     void sigCPUExecutionCapChange();
//     void sigGuestMonitorChange(KGuestMonitorChangedEventType changeType, ulong uScreenId, QRect screenGeo);

	private:
		MachineBridge *machine;
};

#endif //UIMAINEVENTLISTENER_H

