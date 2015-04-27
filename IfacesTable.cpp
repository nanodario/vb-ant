#include "IfacesTable.h"

// #include <Qt>
#include <QTableWidget>
#include <QHeaderView>
#include <QBoxLayout>
#include <QObject>
#include <vector>

#include "Iface.h"

IfacesTable::IfacesTable(QWidget *parent, QBoxLayout *layout) : QTableWidget(parent)
{
	setObjectName(QString::fromUtf8("tableView"));

	QStringList horizontalHeaderLabels = QString("Nome interfaccia;Indirizzo MAC;Indirizzo IP;Maschera sottorete;Nome sottorete").split(";");

	setColumnCount(horizontalHeaderLabels.count());
	setHorizontalHeaderLabels(horizontalHeaderLabels);

	QStringList verticalHeaderLabels = QString("Interfaccia 1;Interfaccia 2;Interfaccia 3;Interfaccia 4;Interfaccia 5;Interfaccia 6;Interfaccia 7;Interfaccia 8").split(";");
	setRowCount(verticalHeaderLabels.count());
	setVerticalHeaderLabels(verticalHeaderLabels);

	QHeaderView *headerView = new QHeaderView(Qt::Horizontal, this);
	headerView->setResizeMode(QHeaderView::Stretch);
	setHorizontalHeader(headerView);
	
	int i;
	for(i = 0; i < IFACES_NUMBER; i++)
		addIface("");
	
	connect(this, SIGNAL(cellChanged(int,int)), this, SLOT(setItemSlot(int,int)));
}

IfacesTable::~IfacesTable()
{

}

int IfacesTable::addIface(QString name, QString mac, QString ip, QString subnetMask, QString subnetName)
{
	Iface *i = new Iface(name, mac);
	ifaces.push_back(i);
	int row = ifaces.size() - 1;
	
	setItem(row, COLUMN_IFACE_NAME, new QTableWidgetItem(/*"ITEM 1"*/));
	setItem(row, COLUMN_MAC, new QTableWidgetItem(/*"ITEM 2"*/));
	setItem(row, COLUMN_IP, new QTableWidgetItem(/*"ITEM 3"*/));
	setItem(row, COLUMN_SUBNETMASK, new QTableWidgetItem(/*"ITEM 4"*/));
	setItem(row, COLUMN_SUBNETNAME, new QTableWidgetItem(/*"ITEM 5"*/));

	item(row, COLUMN_IFACE_NAME)->setText(i->name);
	item(row, COLUMN_MAC)->setText(i->mac);
	item(row, COLUMN_IP)->setText(i->ip);
	item(row, COLUMN_SUBNETMASK)->setText(i->subnetMask);
	item(row, COLUMN_SUBNETNAME)->setText(i->subnetName);
	
	return row;
}

bool IfacesTable::setName(int iface, QString name)
{
	item(iface, COLUMN_IFACE_NAME)->setText(name);
	return true;
}

bool IfacesTable::setMac(int iface, QString mac)
{
	QString old_mac = ifaces.at(iface)->mac;
	bool done = ifaces.at(iface)->setMac(mac);
	if (done)
	{
		QString new_mac = ifaces.at(iface)->mac;
		item(iface, COLUMN_MAC)->setText(new_mac);
	}
	else
		item(iface, COLUMN_MAC)->setText(old_mac);
	
	return done;
}

bool IfacesTable::setIp(int iface, QString ip)
{
	QString old_ip = ifaces.at(iface)->ip;
	bool done = ifaces.at(iface)->setIp(ip);
	if (done)
	{
		QString new_ip = ifaces.at(iface)->ip;
		item(iface, COLUMN_IP)->setText(ip);
	}
	else
		item(iface, COLUMN_IP)->setText(old_ip);

	return done;
}

bool IfacesTable::setSubnetMask(int iface, QString subnetMask)
{
	QString old_subnetMask = ifaces.at(iface)->subnetMask;
	bool done = ifaces.at(iface)->setSubnetMask(subnetMask);
	if (done)
	{
		QString new_subnetMask = ifaces.at(iface)->subnetMask;
		item(iface, COLUMN_SUBNETMASK)->setText(new_subnetMask);
	}
	else
		item(iface, COLUMN_SUBNETMASK)->setText(old_subnetMask);
	
	return done;
}

bool IfacesTable::setSubnetName(int iface, QString subnetName)
{
	item(iface, COLUMN_SUBNETNAME)->setText(subnetName);
	return true;
}

void IfacesTable::setItemSlot(int row, int column)
{
	switch(column)
	{
		case COLUMN_IFACE_NAME:
			setName(row, item(row, COLUMN_IFACE_NAME)->text());
			break;
		case COLUMN_MAC:
			setMac(row, item(row, COLUMN_MAC)->text());
			break;
		case COLUMN_IP:
			setIp(row, item(row, COLUMN_IP)->text());
			break;
		case COLUMN_SUBNETMASK:
			setSubnetMask(row, item(row, COLUMN_SUBNETMASK)->text());
			break;
		case COLUMN_SUBNETNAME:
			setSubnetName(row, item(row, COLUMN_SUBNETNAME)->text());
			break;
	}
}
