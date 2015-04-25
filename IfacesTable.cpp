#include "IfacesTable.h"

// #include <Qt>
#include <QTableWidget>
#include <QHeaderView>
#include <qboxlayout.h>
#include <vector>

#include "Iface.h"

IfacesTable::IfacesTable(QWidget *parent, QBoxLayout *layout)
{
	ifaces_table = new QTableWidget(parent);
	ifaces_table->setObjectName(QString::fromUtf8("tableView"));

	QStringList horizontalHeaderLabels = QString("Nome interfaccia;Indirizzo MAC;Indirizzo IP;Maschera sottorete;Nome sottorete").split(";");

	ifaces_table->setColumnCount(horizontalHeaderLabels.count());
	ifaces_table->setHorizontalHeaderLabels(horizontalHeaderLabels);

	QStringList verticalHeaderLabels = QString("Interfaccia 1;Interfaccia 2;Interfaccia 3;Interfaccia 4;Interfaccia 5;Interfaccia 6;Interfaccia 7;Interfaccia 8").split(";");
	ifaces_table->setRowCount(verticalHeaderLabels.count());
	ifaces_table->setVerticalHeaderLabels(verticalHeaderLabels);

	QHeaderView *headerView = new QHeaderView(Qt::Horizontal, ifaces_table);
	headerView->setResizeMode(QHeaderView::Stretch);
	ifaces_table->setHorizontalHeader(headerView);

	layout->addWidget(ifaces_table);
}

IfacesTable::~IfacesTable()
{

}

int IfacesTable::addIface(QString name)
{
	return addIface(name, "00:aa:11:bb:66:88");
}

int IfacesTable::addIface(QString name, QString mac)
{
	Iface *i = new Iface(name, mac);
	ifaces.push_back(i);
	int row = ifaces.size() - 1;
	
	ifaces_table->setItem(row, 0, new QTableWidgetItem(/*"ITEM 1"*/));
	ifaces_table->setItem(row, 1, new QTableWidgetItem(/*"ITEM 2"*/));
	ifaces_table->setItem(row, 2, new QTableWidgetItem(/*"ITEM 3"*/));
	ifaces_table->setItem(row, 3, new QTableWidgetItem(/*"ITEM 4"*/));
	ifaces_table->setItem(row, 4, new QTableWidgetItem(/*"ITEM 5"*/));
	
	ifaces_table->item(row, 0)->setText(i->name);
	ifaces_table->item(row, 1)->setText(i->mac);
	ifaces_table->item(row, 2)->setText(i->ip);
	ifaces_table->item(row, 3)->setText(i->subnetMask);
	ifaces_table->item(row, 4)->setText(i->subnetName);
	
	return row;
}
