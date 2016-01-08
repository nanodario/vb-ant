/*
 * VB-ANT - VirtualBox - Advanced Network Tool
 * Copyright (C) 2015, 2016  Dario Messina
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

#include "Iface.h"

#include <QStringList>
#include <string>
#include <sstream>
#include <iostream>
#include <stdint.h>
#include <regex>
#include "VirtualBoxBridge.h"

#define IPV6_REGEX "(([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|fe80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::(ffff(:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))"

static QString subnetMaskFromSubnetSize(int subnetSize)
{
	typedef union
	{
		uint64_t subnetMask_ipv6[2];
		uint32_t subnetMask_ipv4[4];
		uint8_t subnetMask_byte[4][4];
	} subnetMask_t;

	subnetMask_t subnetMask;
	subnetMask.subnetMask_ipv6[0] = subnetMask.subnetMask_ipv6[1] = 0;

	for (int i = 31; i > (31 - subnetSize); i--)
		subnetMask.subnetMask_ipv4[0] |= 1 << i;

	std::stringstream ss;
	ss << (int) subnetMask.subnetMask_byte[0][3] << "." << (int) subnetMask.subnetMask_byte[0][2] << "." << (int) subnetMask.subnetMask_byte[0][1] << "." << (int) subnetMask.subnetMask_byte[0][0];

	std::string out;
	ss >> out;
	
	return QString::fromUtf8(out.c_str());
}

#ifdef ENABLE_IPv6
int Iface::subnetSizeFromSubnetMask(QString qSubnetMask)
{
	typedef union
	{
		uint64_t subnetMask_ipv6[2];
		uint32_t subnetMask_ipv4[4];
		uint8_t subnetMask_byte[4][4];
	} subnetMask_t;

	subnetMask_t subnetMask;
	subnetMask.subnetMask_ipv6[0] = subnetMask.subnetMask_ipv6[1] = 0;

	QStringList s_qSubnetMask = qSubnetMask.split(".");
	
	for (int i = 0; i < s_qSubnetMask.size(); i++)
		subnetMask.subnetMask_byte[0][i] = s_qSubnetMask[3-i].toInt();

	int size = 0;
	for (int i = 0; i < 32; i++)
		if(subnetMask.subnetMask_ipv4[0] & (1<<i))
			size++;

	return size;
}
#endif

static bool isValidHexNumber(std::string hex, int min, int max)
{
	int i;
	if (hex.length() == 0)
		return false;
	for(i = 0; i < hex.length(); i++)
		if(!isxdigit(hex.c_str()[i]))
			return false;
		
	unsigned int x;
	std::stringstream ss;
	
	ss << std::hex << hex;
	ss >> x;
	
	return (x >= min && x <= max);
}

static bool isValidDecNumber(std::string num, int min, int max)
{
	int i;
	if (num.length() == 0)
		return false;
	
	for (i = 0; i < num.length(); i++)
		if(!isdigit(num.c_str()[i]))
			return false;
		
	unsigned int x;
	std::stringstream ss;
	
	ss << std::dec << num;
	ss >> x;
	
	return (x >= min && x <= max);
}

#ifdef CONFIGURABLE_IP
Iface::Iface(bool enabled, QString mac, bool cableConnected, uint32_t attachmentType, QString attachmentData, QString name, QString ip, QString subnetMask)
: enabled(enabled), cableConnected(cableConnected)
#else
Iface::Iface(bool enabled, QString mac, bool cableConnected, uint32_t attachmentType, QString attachmentData, QString name)
: enabled(enabled), cableConnected(cableConnected)
#endif
{
	setName(name);
	last_valid_name = name;
	setMac(mac);
	setAttachmentType(attachmentType);
	setAttachmentData(attachmentData);
#ifdef CONFIGURABLE_IP
	setIp(ip);
	setSubnetMask(subnetMask);
#endif
}

Iface::Iface(serializable_iface_t serializable_iface)
: enabled(serializable_iface.enabled), cableConnected(serializable_iface.cableConnected)
{
	setName(QString::fromUtf8(serializable_iface.name));
	last_valid_name = QString::fromUtf8(serializable_iface.last_valid_name);
	setMac(QString::fromUtf8(serializable_iface.mac));
	setAttachmentType(serializable_iface.attachmentType);
	setAttachmentData(QString::fromUtf8(serializable_iface.attachmentData));
#ifdef CONFIGURABLE_IP
	setIp(serializable_iface.ip);
	setSubnetMask(serializable_iface.subnetMask);
#endif
}

Iface::~Iface()
{

}

bool Iface::setName(QString _name)
{
	if(isValidName(_name, true))
	{
		name = _name;
		return true;
	}
	return false;
}

bool Iface::setMac(QString _mac)
{
	if(isValidMac(_mac))
	{
		mac = formatMac(_mac);
		return true;
	}
	mac = "";
	return false;
}

#ifdef CONFIGURABLE_IP
bool Iface::setIp(QString _ip)
{
	if(
#ifdef ENABLE_IPv6
		isValidIPv6(_ip) ||
#endif
		isValidIPv4(_ip))
	{
		ip = _ip;
		return true;
	}
	ip = "";
	return false;	
}

bool Iface::setSubnetMask(QString _subnetMask)
{
	if(isValidSubnetMask(_subnetMask, ip))
	{
#ifdef VALIDATE_IP
		if(_subnetMask.length() == 0)
			subnetMask = _subnetMask;
		else if(_subnetMask.split(".").count() == 1
#ifdef ENABLE_IPv6
			&& !isValidIPv6(ip)
#endif
		)
			subnetMask = subnetMaskFromSubnetSize(_subnetMask.toInt());
		else
#endif
			subnetMask = _subnetMask;
		return true;
	}
	else if(subnetMask == _subnetMask)
	{
		subnetMask = "";
		return true;
	}
	return false;
}
#endif

bool Iface::setAttachmentData(QString _attachmentData)
{
	if(isValidName(_attachmentData, true))
	{
		attachmentData = _attachmentData;
		return true;
	}
	return false;
}

bool Iface::setAttachmentType(uint32_t _attachmentType)
{
	if(isValidAttachmentType(_attachmentType))
	{
		attachmentType = _attachmentType;
		return true;
	}
	
	return false;
}

bool Iface::isValidName(QString name, bool blankAllowed)
{
	if(blankAllowed && name.length() == 0)
		return true;
	
	int i;
	for (i = 0; i < name.length(); i++)
		if(!isalnum(name.toStdString().c_str()[i]))
			return false;

	return true;
}

/*
 * What is a valid MAC address?
 * 
 * A MAC address is a unique identifier assigned to most network adapters or
 * network interface cards (NICs) by the manufacturer for identification,
 * IEEE 802 standards use 48 bites or 6 bytes to represent a MAC address.
 * This format gives 281,474,976,710,656 possible unique MAC addresses.
 * 
 * IEEE 802 standards define 3 commonly used formats to print a MAC address
 * in hexadecimal digits:
 * 
 *    Six groups of two hexadecimal digits separated by hyphens (-),
 *       like 01-23-45-67-89-ab
 *    Six groups of two hexadecimal digits separated by colons (:),
 *       like 01:23:45:67:89:ab
 *    Three groups of four hexadecimal digits separated by dots (.),
 *       like 0123.4567.89ab
 * 
 * [from http://sqa.fyicenter.com/Online_Test_Tools/MAC_Address_Format_Validator.php]
 * 
 * Another accepted format is twelve hexadecimal digits, like 0123456789ab
 */
bool Iface::isValidMac(QString mac)
{
	if (mac.length() == 0)
		return true;

	int i;
	QStringList s_mac;

	s_mac = mac.split(":");
	if (s_mac.count() == 6)
	{
		for (i = 0; i < 6; i++)
			if (!isValidHexNumber(s_mac.at(i).toStdString(), 0x00, 0xFF))
				return false;
		return true;
	}

	s_mac = mac.split("-");
	if (s_mac.count() == 6)
	{
		for (i = 0; i < 6; i++)
			if (!isValidHexNumber(s_mac.at(i).toStdString(), 0x00, 0xFF))
			return false;
		return true;
	}

	s_mac = mac.split(".");
	if (s_mac.count() == 3)
	{
		for (i = 0; i < 3; i++)
			if (s_mac.at(i).length() < 4 || !isValidHexNumber(s_mac.at(i).toStdString(), 0x0000, 0xFFFF))
				return false;
		return true;
	}
	
	std::string str_mac = mac.toStdString();
	if (str_mac.length() == 12)
	{
		for (i = 0; i < 6; i++)
			if (!isValidHexNumber(str_mac.substr(i*2, 2), 0x00 , 0xFF))
				return false;
		return true;
	}

	return false;
}

QString Iface::formatMac(QString mac)
{
	if (isValidMac(mac))
	{
		int i;
		QStringList s_mac;
		std::stringstream ss;

		s_mac = mac.split(":");
		if (s_mac.count() == 6)
		{
			std::string temp;
			for (i = 0; i < 6; i++)
			{
				temp = s_mac.at(i).toStdString();
				
				if (temp.length() < 2)
					ss << std::string("0");
				
				ss << temp;
				
				if(i != 5)
					ss << ":";
			}
			
			std::string mac_out;
			ss >> mac_out;
			
			return (QString::fromStdString(mac_out)).toUpper();
		}
		
		s_mac = mac.split("-");
		if (s_mac.count() == 6)
		{
			std::string temp;
			for (i = 0; i < 6; i++)
			{
				temp = s_mac.at(i).toStdString();
				
				if (temp.length() < 2)
					ss << std::string("0");
				
				ss << temp;
				
				if(i != 5)
					ss << ":";
			}

			std::string mac_out;
			ss >> mac_out;
			
			return (QString::fromStdString(mac_out)).toUpper();
		}

		s_mac = mac.split(".");
		if (s_mac.count() == 3)
		{
			std::string temp;
			for (i = 0; i < 3; i++)
			{
				temp = s_mac.at(i).toStdString();
				if (temp.length() == 4)
					ss << temp.substr(0,2).append(":").append(temp.substr(2,2));
				if(i != 2)
					ss << ":";
			}

			std::string mac_out;
			ss >> mac_out;

			return (QString::fromStdString(mac_out)).toUpper();
		}
		
		std::string str_mac = mac.toStdString();
		if (str_mac.length() == 12)
		{
			std::string temp;
			for (i = 0; i < 6; i++)
			{
				ss << str_mac.substr((i*2),2);
				if(i != 5)
					ss << ":";
			}
			
			std::string mac_out;
			ss >> mac_out;
			
			return (QString::fromStdString(mac_out)).toUpper();
		}
	}
	return QString::fromUtf8("");
}

#if defined(CONFIGURABLE_IP) && defined(VALIDATE_IP)
bool Iface::isValidIPv4(QString ip)
{
	if (ip.length() == 0)
		return true;

	int i;
#ifdef EXCLUDE_RESERVED_IP
	int ip_a[] = {0, 0, 0, 0};
#endif
	
	QStringList s_ip = ip.split(".");
	if (s_ip.count() == 4)
	{
		for (i = 0; i < 4; i++)
		{
			if (!isValidDecNumber(s_ip.at(i).toStdString(), 0, 255))
				return false;
#ifdef EXCLUDE_RESERVED_IP
			ip_a[i] = s_ip.at(i).toShort();
#endif
		}
	}

#ifdef EXCLUDE_RESERVED_IP
	if
	(
	   // Used for broadcast messages to the current network as specified by RFC 1700, page 4
	   (ip_a[0] == 0) ||

	   // Used for loopback addresses to the local host, as specified by RFC 990.
	   (ip_a[0] == 127) ||

	   /* Used for link-local addresses between two hosts on a single link
	    * when no IP address is otherwise specified, such as would have
	    * normally been retrieved from a DHCP server, as specified by RFC 3927.
	    */
	   (ip_a[0] == 169 && ip_a[1] == 254) ||

	   // Reserved for the "limited broadcast" destination address, as specified by RFC 6890.
	   (ip_a[0] == 255 && ip_a[1] == 255 && ip_a[2] == 255 && ip_a[3] == 255)
	)
		return false;
#endif
	
	return true;
}

bool Iface::isValidSubnetMask(QString subnetMask, QString ip)
{
	if (subnetMask.length() == 0)
		return true;

	int i;
	int smask_a[] = {0, 0, 0, 0};

	QStringList s_smask = subnetMask.split(".");
	if (s_smask.count() == 1)
	{
		if(isValidDecNumber(s_smask.at(0).toStdString(), 0, 32)
#ifdef ENABLE_IPv6
		|| (isValidIPv6(ip) && isValidDecNumber(s_smask.at(0).toStdString(), 0, 64))
#endif
		)
			return true;
		return false;
	}

	if (s_smask.count() == 4)
	{
		uint32_t subnetMask_32 = 0;
		for (i = 0; i < 4; i++)
		{
			if (!isValidDecNumber(s_smask.at(i).toStdString(), 0, 255))
				return false;
			subnetMask_32 += (uint32_t) (s_smask.at(i).toInt() << 8 * (3-i));
		}

		uint32_t checkMask = 0;
		for (i = 31; i >= 0; i--)
		{
			if (subnetMask_32 == checkMask)
				return true;
			checkMask |= 1 << i;
		}
		if (subnetMask_32 == checkMask)
			return true;
	}
	return false;
}
#endif

#if defined(CONFIGURABLE_IP) && defined(ENABLE_IPv6)
bool Iface::isValidIPv6(QString ip)
{
	return std::regex_match (ip.toStdString().c_str(), std::regex(IPV6_REGEX));
}
#endif


Iface *Iface::copyIface()
{
#ifdef CONFIGURABLE_IP
	return new Iface(enabled, mac, cableConnected, attachmentType, attachmentData, name, ip, subnetMask);
#else
	return new Iface(enabled, mac, cableConnected, attachmentType, attachmentData, name);
#endif
}

serializable_iface_t Iface::getSerializableIface()
{
	serializable_iface_t serializable_iface;
	memset(&serializable_iface, 0, sizeof(serializable_iface_t));
	strcpy(serializable_iface.last_valid_name, last_valid_name.toStdString().c_str());
	strcpy(serializable_iface.name, name.toStdString().c_str());
	strcpy(serializable_iface.mac, mac.toStdString().c_str());
	strcpy(serializable_iface.attachmentData, attachmentData.toStdString().c_str());

#ifdef CONFIGURABLE_IP	
	strcpy(serializable_iface.ip, ip.toStdString().c_str());
	strcpy(serializable_iface.subnetMask, subnetMask.toStdString().c_str());
#endif
	
	serializable_iface.attachmentType = attachmentType;
	serializable_iface.cableConnected = cableConnected;
	serializable_iface.enabled = enabled;
	
	return serializable_iface;
}

bool Iface::isValidAttachmentType(uint32_t _attachmentType)
{
	switch(_attachmentType)
	{
		case NetworkAttachmentType::Null:
		case NetworkAttachmentType::Bridged:
		case NetworkAttachmentType::Generic:
		case NetworkAttachmentType::HostOnly:
		case NetworkAttachmentType::Internal:
		case NetworkAttachmentType::NAT:
		case NetworkAttachmentType::NATNetwork:
			return true;
	}
	return false;
}
