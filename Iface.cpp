#include "Iface.h"

#include <QStringList>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdint.h>

Iface::Iface(QString name)
: name(name),/* mac(QString::fromUtf8("")), ip(QString::fromUtf8("")), subnetMask(QString::fromUtf8("")),*/ subnetName(QString::fromUtf8(""))
{
	
}

Iface::Iface(QString name, QString _mac)
: name(name), /*mac(mac),*/ ip(QString::fromUtf8("")), subnetMask(QString::fromUtf8("")), subnetName(QString::fromUtf8(""))
{
	if(isValidMac(_mac))
		mac = _mac;
	else
	{
		mac = /*QString("MAC ERRATO")*/ _mac;
		ip = QString("Mac errato");
	}
}

Iface::~Iface()
{

}

bool Iface::setMac(QString _mac)
{
	if(isValidMac(_mac))
	{
		mac = _mac;
		return true;
	}
	return false;
}


bool Iface::isValidHexNumber(std::string hex, int min, int max)
{
	int i;
	for(i = 0; i < hex.length(); i++)
		if(!isxdigit(hex.c_str()[i]))
			return false;

	unsigned int x;
	std::string reverse_test;
	std::stringstream ss;

	ss << std::hex << hex;
	ss >> x;

	return (x >= min && x <= max);
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
 */
bool Iface::isValidMac(QString mac)
{
	bool separatorMatch = false;
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
			if (!isValidHexNumber(s_mac.at(i).toStdString(), 0x0000, 0xFFFF))
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

bool Iface::isValidIp(QString ip)
{
	return false;
}

bool Iface::isValidSubnetMask(QString subnetMask)
{
	return false;
}
