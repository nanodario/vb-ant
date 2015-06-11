#ifndef IFACE_H
#define IFACE_H

#define EXCLUDE_RESERVED_IP

#include <QString>

class Iface
{
	public:
#ifdef CONFIGURABLE_IP
		Iface(bool enabled = false, QString name = "", QString mac = "", QString ip = "", QString subnetMask = "", QString subnetName = "");
#else
		Iface(bool enabled = false, QString name = "", QString mac = "", QString subnetName = "");
#endif
		
		virtual ~Iface();
		bool setName(QString name);
		bool setMac(QString mac);
#ifdef CONFIGURABLE_IP
		bool setIp(QString ip);
		bool setSubnetMask(QString subnetMask);
#endif
		bool setSubnetName(QString subnetName);

		static QString formatMac(QString mac);
		static bool isValidName(QString name, bool blankAllowed = false);
		static bool isValidMac(QString mac);
#ifdef CONFIGURABLE_IP
		static bool isValidIp(QString ip);
		static bool isValidSubnetMask(QString subnetMask);
#endif
		
		static QString generateMac();

		QString name, mac,
#ifdef CONFIGURABLE_IP
		ip, subnetMask,
#endif
		subnetName;
		bool enabled;
// 	private:
};

#endif //IFACE_H
