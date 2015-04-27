#include "Iface.h"

#include <QStringList>
#include <string>
#include <sstream>
#include <iostream>
#include <stdint.h>

static QString subnetMaskFromSubnetSize(int subnetSize)
{
	int i;
	uint32_t subnetMask_bits = 0;
	int subnetMask_dotted[] = {0, 0, 0, 0};
	
	for (i = 31; i > (31 - subnetSize); i--)
		subnetMask_bits |= 1 << i;
	
	for(i = 0; i < 4; i++)
		subnetMask_dotted[i] = (subnetMask_bits >> ((3-i)*8)) & 0xFF;

	std::stringstream ss;
	ss << subnetMask_dotted[0] << "." << subnetMask_dotted[1] << "." << subnetMask_dotted[2] << "." << subnetMask_dotted[3];
	
	std::string out;
	ss >> out;
	
	return QString::fromUtf8(out.c_str());
}

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
		if(num.c_str()[i] < '0' || num.c_str()[i] > '9')
			return false;
		
		unsigned int x;
	std::stringstream ss;
	
	ss << std::dec << num;
	ss >> x;
	
	return (x >= min && x <= max);
}

Iface::Iface(QString name)
: name(name), mac(QString::fromUtf8("")), ip(QString::fromUtf8("")), subnetMask(QString::fromUtf8("")), subnetName(QString::fromUtf8(""))
{
	
}

Iface::Iface(QString name, QString _mac)
: name(name), mac(QString::fromUtf8("")), ip(QString::fromUtf8("")), subnetMask(QString::fromUtf8("")), subnetName(QString::fromUtf8(""))
{
	setMac(_mac);
}

Iface::~Iface()
{

}

bool Iface::setName(QString _name)
{
	name = _name;
	return true;
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

bool Iface::setIp(QString _ip)
{
	if(isValidIp(_ip))
	{
		ip = _ip;
		return true;
	}
	ip = "";
	return false;	
}

bool Iface::setSubnetMask(QString _subnetMask)
{
	if(isValidSubnetMask(_subnetMask))
	{
		if(_subnetMask.split(".").count() == 1 && _subnetMask.length() != 0)
			subnetMask = subnetMaskFromSubnetSize(_subnetMask.toInt());
		else
			subnetMask = _subnetMask;
		return true;
	}
	return false;
}

bool Iface::setSubnetName(QString _subnetName)
{
	subnetName = _subnetName;
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
			return mac.toUpper();

		s_mac = mac.split("-");
		if (s_mac.count() == 6)
		{
			std::string temp;
			for (i = 0; i < 6; i++)
			{
				temp = s_mac.at(i).toStdString();
				if (temp.length() < 2)
					ss << std::string("0").append(temp);
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

/*
 * 
 */
bool Iface::isValidIp(QString ip)
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

bool Iface::isValidSubnetMask(QString subnetMask)
{
	if (subnetMask.length() == 0)
		return true;

	int i;
	int smask_a[] = {0, 0, 0, 0};

	QStringList s_smask = subnetMask.split(".");
	if (s_smask.count() == 1)
	{
		if(!isValidDecNumber(s_smask.at(0).toStdString(), 0, 32))
			return false;
		return true;
	}

	if (s_smask.count() == 4)
	{
		for (i = 0; i < 4; i++)
			if (!isValidDecNumber(s_smask.at(i).toStdString(), 0, 255))
				return false;
		return true;
	}
	return false;
}
