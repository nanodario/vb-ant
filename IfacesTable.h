#ifndef IFACETABLE_H
#define IFACETABLE_H

#include <QBoxLayout>
#include <QTableWidget>
#include <vector>

#include "Iface.h"

#define IFACES_NUMBER 8

#define COLUMN_IFACE_NAME	0
#define COLUMN_MAC		1
#define COLUMN_IP		2
#define COLUMN_SUBNETMASK	3
#define COLUMN_SUBNETNAME	4

class IfacesTable :  public QTableWidget
{
	Q_OBJECT
	
	public:
		IfacesTable(QWidget *parent, QBoxLayout *layout);
		~IfacesTable();
		int addIface(QString name = "", QString mac = "", QString ip = "", QString subnetMask = "", QString subnetName = "");
		
		bool setName(int iface, QString name);
		bool setMac(int iface, QString mac);
		bool setIp(int iface, QString ip);
		bool setSubnetMask(int iface, QString subnetMask);
		bool setSubnetName(int iface, QString subnetName);
		
		QStringList getIfaceInfo(int iface);
		void removeIface(int iface_index);
		
	private slots:
		void setItemSlot(int row, int column);
		
	protected:
		std::vector<Iface*> ifaces;
		QTableWidget *ifaces_table;
		
// 	private:
};

#endif
