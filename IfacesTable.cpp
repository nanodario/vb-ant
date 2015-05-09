#include "IfacesTable.h"

// #include <Qt>
#include <QCheckBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QBoxLayout>
#include <QToolButton>
#include <QObject>
#include <QLineEdit>
#include <vector>

#include <iostream>
// #include "VBoxGlobal.h"
// #include "HostImpl.h"

#include "Iface.h"

MacWidgetField::MacWidgetField(QWidget *parent, MacWidgetFieldBridge *bridge) : QWidget(parent)
{
	QHBoxLayout *horizontalLayout = new QHBoxLayout(this);
	horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
	horizontalLayout->setContentsMargins(0, 0, 0, 0);

	lineEdit = new QLineEdit(this);
	lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
	horizontalLayout->addWidget(lineEdit);

	button = new QToolButton(this);
	button->setObjectName(QString::fromUtf8("button"));
	button->setAutoRaise(true);
	button->setIcon(QIcon(QString::fromUtf8(":/resources/refresh_16px.png")));
	button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	horizontalLayout->addWidget(button);
	
	setLayout(horizontalLayout);
	
	connect(lineEdit, SIGNAL(editingFinished()), bridge, SLOT(editingFinishedSlot()));
	connect(button, SIGNAL(released()), bridge, SLOT(releasedSlot()));
}

MacWidgetField::~MacWidgetField()
{
	delete(button);
	delete(lineEdit);
}

void MacWidgetField::setText(const QString &text)
{
	lineEdit->setText(text);
}

MacWidgetFieldBridge::MacWidgetFieldBridge(int row, IfacesTable *destination) : QWidget()
, row(row), destination(destination)
{
}

void MacWidgetFieldBridge::editingFinishedSlot()
{
	destination->setMac(row, ((QLineEdit *)sender())->text());
}

void MacWidgetFieldBridge::releasedSlot()
{
	destination->generateMac(row);
}

IfacesTable::IfacesTable(QWidget *parent, QBoxLayout *layout) : QTableWidget(parent)
{
	setObjectName(QString::fromUtf8("tableView"));

	QStringList horizontalHeaderLabels = QString("Abilita;Nome interfaccia;Indirizzo MAC;Indirizzo IP;Maschera sottorete;Nome sottorete").split(";");

	setColumnCount(horizontalHeaderLabels.count());
	setHorizontalHeaderLabels(horizontalHeaderLabels);

	QStringList verticalHeaderLabels = QString("Interfaccia 1;Interfaccia 2;Interfaccia 3;Interfaccia 4;Interfaccia 5;Interfaccia 6;Interfaccia 7;Interfaccia 8").split(";");
	setRowCount(verticalHeaderLabels.count());
	setVerticalHeaderLabels(verticalHeaderLabels);

	QHeaderView *headerView = new QHeaderView(Qt::Horizontal, this);
	headerView->setResizeMode(QHeaderView::Stretch);
	setHorizontalHeader(headerView);
	horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	horizontalHeader()->setStretchLastSection(true);
	
	int row;
	for(row = 0; row < IFACES_NUMBER; row++)
	{
		QString name = QString("test%1").arg(row);
		QString mac = QString("0000.0000.000%1").arg(row);
		QString ip = QString("10.10.10.%1").arg(row);
		QString subnetMask = QString("%1").arg(row);
		QString subnetName = QString("subnet%1").arg(row);
		addIface(true, name, mac, ip, subnetMask, subnetName); //TODO
	}

	connect(this, SIGNAL(cellChanged(int,int)), this, SLOT(cellChangedSlot(int,int)));
	connect(this, SIGNAL(cellClicked(int,int)), this, SLOT(cellClickedSlot(int, int)));
}

IfacesTable::~IfacesTable()
{
	
}

int IfacesTable::addIface(bool enabled, QString name, QString mac, QString ip, QString subnetMask, QString subnetName)
{
	Iface *i = new Iface(enabled, name, mac, ip, subnetMask, subnetName);
	ifaces.push_back(i);
	int row = ifaces.size() - 1;
	
	QTableWidgetItem *qtablewidgetitem = new QTableWidgetItem(QString::fromUtf8(""));
	qtablewidgetitem->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
	qtablewidgetitem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
	qtablewidgetitem->setTextAlignment(Qt::AlignHCenter);

	setItem(row, COLUMN_IFACE_ENABLED, qtablewidgetitem);
	setItem(row, COLUMN_IFACE_NAME, new QTableWidgetItem(/*"ITEM 1"*/));
	setCellWidget(row, COLUMN_MAC, new MacWidgetField(this/*"ITEM 2"*/, new MacWidgetFieldBridge(row, this)));
	setItem(row, COLUMN_IP, new QTableWidgetItem(/*"ITEM 3"*/));
	setItem(row, COLUMN_SUBNETMASK, new QTableWidgetItem(/*"ITEM 4"*/));
	setItem(row, COLUMN_SUBNETNAME, new QTableWidgetItem(/*"ITEM 5"*/));

	setStatus(row, enabled);

	item(row, COLUMN_IFACE_NAME)->setText(i->name);

// 	QToolButton *newMacButton = new QToolButton();
// 	newMacButton->setIcon(QIcon(QString::fromUtf8(":/resources/refresh_16px.png")));
// 	newMacButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
	
// 	item(row, COLUMN_MAC);
// 	item(row, COLUMN_MAC)->setText(i->mac);
	((MacWidgetField *)cellWidget(row, COLUMN_MAC))->setText(i->mac);
	item(row, COLUMN_IP)->setText(i->ip);
	item(row, COLUMN_SUBNETMASK)->setText(i->subnetMask);
	item(row, COLUMN_SUBNETNAME)->setText(i->subnetName);
	
	int col;
	for (col = 0; col < columnCount(); col++)
		if(col != COLUMN_MAC)
			item(row, col)->setTextAlignment(Qt::AlignCenter);
		else
			((MacWidgetField *)cellWidget(row, col))->lineEdit->setAlignment(Qt::AlignCenter);
	
	return row;
}

bool IfacesTable::setStatus(int iface, bool checked)
{
	int col;
	for (col = 1; col < columnCount(); col++)
	{
		Qt::ItemFlags flags;
		if(col != COLUMN_MAC)
		{
			flags = item(iface, col)->flags();
			if (checked)
				flags |= Qt::ItemIsEnabled;
			else
				flags &= ~Qt::ItemIsEnabled;
			item(iface, col)->setFlags(flags);
		}
		else
		{
			MacWidgetField *w = (MacWidgetField *)cellWidget(iface, col);
			w->button->setEnabled(checked);
			w->lineEdit->setEnabled(checked);
		}
	}

	ifaces.at(iface)->enabled = checked;
	return true;
}


bool IfacesTable::setName(int iface, QString name)
{
	ifaces.at(iface)->setName(name);
	QString new_name = ifaces.at(iface)->name;
	item(iface, COLUMN_IFACE_NAME)->setText(new_name);
	return true;
}

bool IfacesTable::setMac(int iface, QString mac)
{
	QString old_mac = ifaces.at(iface)->mac;
	bool done = ifaces.at(iface)->setMac(mac);
	if (done)
	{
		QString new_mac = ifaces.at(iface)->mac;
		((MacWidgetField *)cellWidget(iface, COLUMN_MAC))->setText(new_mac);
	}
	else
		((MacWidgetField *)cellWidget(iface, COLUMN_MAC))->setText(old_mac);
	
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
	ifaces.at(iface)->setSubnetName(subnetName);
	QString new_subnetName = ifaces.at(iface)->subnetName;
	item(iface, COLUMN_SUBNETNAME)->setText(new_subnetName);
	return true;
}

void IfacesTable::generateMac(int iface)
{
// 	setMac(iface, vboxGlobal().host().GenerateMACAddress()); //FIXME
	setMac(iface, QString::fromUtf8("1234567890AB"));
}

QStringList IfacesTable::getIfaceInfo(int iface)
{
	QStringList iface_info;
	iface_info.push_back(ifaces.at(iface)->name);
	iface_info.push_back(ifaces.at(iface)->mac);
	iface_info.push_back(ifaces.at(iface)->ip);
	iface_info.push_back(ifaces.at(iface)->subnetMask);
	iface_info.push_back(ifaces.at(iface)->subnetName);
	
	return iface_info;
}

void IfacesTable::cellChangedSlot(int row, int column)
{
	switch(column)
	{
		case COLUMN_IFACE_ENABLED:
			setStatus(row, item(row, COLUMN_IFACE_ENABLED)->checkState() == Qt::Checked);
			break;
		case COLUMN_IFACE_NAME:
			setName(row, item(row, COLUMN_IFACE_NAME)->text());
			break;
		case COLUMN_MAC:
			std::cout << "latuma" << std::endl;
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

void IfacesTable::cellClickedSlot(int row, int column)
{
	if (column == COLUMN_MAC)
	{
		std::cout << "la tuma" << std::endl;
	}
}
