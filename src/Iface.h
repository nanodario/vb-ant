/*
 * VB-ANT - VirtualBox - Advanced Network Tool
 * Copyright (C) 2015  Dario Messina
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

#ifndef IFACE_H
#define IFACE_H

#define EXCLUDE_RESERVED_IP

#include <QString>
#include <stdint.h>
#include "VirtualBoxBridge.h"

class Iface
{
	public:
#ifdef CONFIGURABLE_IP
		Iface(bool enabled = false, QString mac = "", bool cableConnected = false, uint32_t attachmentType = NetworkAttachmentType::Null, QString attachmentData = "", QString name = "", QString ip = "", QString subnetMask = "");
#else
		Iface(bool enabled = false, QString mac = "", bool cableConnected = false, uint32_t attachmentType = NetworkAttachmentType::Null, QString attachmentData = "", QString name = "");
#endif
		
		virtual ~Iface();
		bool setName(QString name);
		bool setMac(QString mac);
		bool setAttachmentType(uint32_t attachmentType);
		bool setAttachmentData(QString attachmentData);
#ifdef CONFIGURABLE_IP
		bool setIp(QString ip);
		bool setSubnetMask(QString subnetMask);
#endif

		static QString formatMac(QString mac);
		static bool isValidName(QString name, bool blankAllowed = false);
		static bool isValidMac(QString mac);
		static bool isValidAttachmentType(uint32_t attachmentType);
#ifdef CONFIGURABLE_IP
		static bool isValidIPv4(QString ip);
#ifdef ENABLE_IPv6
		static bool isValidIPv6(QString ip);
#endif
		static bool isValidSubnetMask(QString subnetMask, QString ip = QString::fromUtf8(""));
		static int subnetSizeFromSubnetMask(QString qSubnetMask);
#endif

		inline bool operator==(const Iface *i) const { return mac == i->mac; };
		inline bool operator!=(const Iface *i) const { return !operator==(i); };
		Iface *copyIface();
		
		QString last_valid_name, name, mac, attachmentData;
#ifdef CONFIGURABLE_IP
		QString ip, subnetMask;
#endif
		uint32_t attachmentType;
		bool enabled, cableConnected;
// 	private:
};

#endif //IFACE_H
