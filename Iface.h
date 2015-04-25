#ifndef IFACE_H
#define IFACE_H

#include <QString>
#include <string>

class Iface
{
	public:
		Iface(QString name);
		Iface(QString name, QString mac);
		virtual ~Iface();
		bool setMac(QString mac);
		QString name, mac, ip, subnetMask, subnetName;
		
	private:
		bool isValidHexNumber(std::string hex, int min, int max);
		bool isValidMac(QString mac);
		bool isValidIp(QString ip);
		bool isValidSubnetMask(QString subnetMask);
};

#endif //IFACE_H
