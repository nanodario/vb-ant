#ifndef IFACE_H
#define IFACE_H

#define EXCLUDE_RESERVED_IP

#include <QString>

class Iface
{
	public:
		Iface(bool enabled = false, QString name = "", QString mac = "", QString ip = "", QString subnetMask = "", QString subnetName = "");
		virtual ~Iface();
		bool setName(QString name);
		bool setMac(QString mac);
		bool setIp(QString ip);
		bool setSubnetMask(QString subnetMask);
		bool setSubnetName(QString subnetName);
		QString name, mac, ip, subnetMask, subnetName;
		bool enabled;

	private:
		bool isValidName(QString name);
		bool isValidMac(QString mac);
		QString formatMac(QString mac);
		bool isValidIp(QString ip);
		bool isValidSubnetMask(QString subnetMask);
};

#endif //IFACE_H
