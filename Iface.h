#ifndef IFACE_H
#define IFACE_H

#define EXCLUDE_RESERVED_IP

#include <QString>
#include <string>

class Iface
{
	public:
		Iface(QString name);
		Iface(QString name, QString mac);
		virtual ~Iface();
		bool setName(QString name);
		bool setMac(QString mac);
		bool setIp(QString ip);
		bool setSubnetMask(QString subnetMask);
		bool setSubnetName(QString subnetName);
		QString name, mac, ip, subnetMask, subnetName;

	private:
		bool isValidMac(QString mac);
		QString formatMac(QString mac);
		bool isValidIp(QString ip);
		bool isValidSubnetMask(QString subnetMask);
};

#endif //IFACE_H
