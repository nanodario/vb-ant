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
		Iface(bool enabled = false, QString mac = "", uint32_t attachmentType = NetworkAttachmentType::Null, QString name = "", QString ip = "", QString subnetMask = "", QString subnetName = "");
#else
		Iface(bool enabled = false, QString mac = "", uint32_t attachmentType = NetworkAttachmentType::Null, QString name = "", QString subnetName = "");
#endif
		
		virtual ~Iface();
		bool setName(QString name);
		bool setMac(QString mac);
#ifdef CONFIGURABLE_IP
		bool setIp(QString ip);
		bool setSubnetMask(QString subnetMask);
#endif
		bool setSubnetName(QString subnetName);
		bool setAttachmentType(uint32_t attachmentType);

		static QString formatMac(QString mac);
		static bool isValidName(QString name, bool blankAllowed = false);
		static bool isValidMac(QString mac);
#ifdef CONFIGURABLE_IP
		static bool isValidIp(QString ip);
		static bool isValidSubnetMask(QString subnetMask);
#endif
		static bool isValidAttachmentType(uint32_t attachmentType);

		inline bool operator==(const Iface *i) const { return mac == i->mac; };
		inline bool operator!=(const Iface *i) const { return !operator==(i); };
		Iface *copyIface();
		
		QString name, mac,
#ifdef CONFIGURABLE_IP
		ip, subnetMask,
#endif
		subnetName;
		uint32_t attachmentType;
		bool enabled;
// 	private:
};

#endif //IFACE_H
