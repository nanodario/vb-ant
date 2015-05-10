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

		static QString formatMac(QString mac);
		static bool isValidName(QString name);
		static bool isValidMac(QString mac);
		static bool isValidIp(QString ip);
		static bool isValidSubnetMask(QString subnetMask);

		QString name, mac, ip, subnetMask, subnetName;
		bool enabled;
// 	private:
};

#endif //IFACE_H
