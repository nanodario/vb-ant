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
#include "VMTabSettings.h"

#include "Iface.h"

MacWidgetField::MacWidgetField(QWidget *parent, int row, IfacesTable *destination) : QWidget(parent)
, row(row), destination(destination)
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
	
	connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(editingFinishedSlot()));
	connect(button, SIGNAL(released()), this, SLOT(releasedSlot()));
}

MacWidgetField::~MacWidgetField()
{
	disconnect(lineEdit, SIGNAL(editingFinished()), this, SLOT(editingFinishedSlot()));
	disconnect(button, SIGNAL(released()), this, SLOT(releasedSlot()));
	delete button;
	delete lineEdit;
	delete layout();
}

void MacWidgetField::setText(const QString &text)
{
	lineEdit->setText(text);
}

void MacWidgetField::editingFinishedSlot()
{
	destination->setMac(row, ((QLineEdit *)sender())->text());
}

void MacWidgetField::releasedSlot()
{
	destination->generateMac(row);
}

IfaceEnableCheckBox::IfaceEnableCheckBox(QWidget *parent, int row, IfacesTable *destination) : QWidget(parent)
, row(row), destination(destination)
{
	QHBoxLayout *horizontalLayout = new QHBoxLayout(this);
	horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
	horizontalLayout->setContentsMargins(0, 0, 0, 0);
	
	checkbox = new QCheckBox(this);
	horizontalLayout->addWidget(checkbox);
	horizontalLayout->setAlignment(Qt::AlignCenter);
	
	setLayout(horizontalLayout);
	
	connect(checkbox, SIGNAL(toggled(bool)), this, SLOT(toggledSlot(bool)));
}

IfaceEnableCheckBox::~IfaceEnableCheckBox()
{
	disconnect(checkbox, SIGNAL(toggled(bool)), this, SLOT(toggledSlot(bool)));
	delete layout();
	delete checkbox;
}

void IfaceEnableCheckBox::setCheckState(Qt::CheckState checked)
{
	checkbox->setCheckState(checked);
}

void IfaceEnableCheckBox::toggledSlot(bool checked)
{
	destination->setStatus(row, checked ? Qt::Checked : Qt::Unchecked);
}

IfacesTable::IfacesTable(QWidget *parent, QBoxLayout *layout, VirtualBoxBridge *vboxbridge) : QTableWidget(parent)
, vboxbridge(vboxbridge)
{
	setObjectName(QString::fromUtf8("tableView"));

#ifdef CONFIGURABLE_IP
	QStringList horizontalHeaderLabels = QString("Abilita;Indirizzo MAC;Nome interfaccia;Indirizzo IP;Maschera sottorete;Nome sottorete").split(";");
#else
	QStringList horizontalHeaderLabels = QString("Abilita;Indirizzo MAC;Nome interfaccia;Nome sottorete").split(";");
#endif

	setColumnCount(horizontalHeaderLabels.count());
	setHorizontalHeaderLabels(horizontalHeaderLabels);

	QStringList verticalHeaderLabels = QString("Interfaccia 1;Interfaccia 2;Interfaccia 3;Interfaccia 4;Interfaccia 5;Interfaccia 6;Interfaccia 7;Interfaccia 8").split(";");
	setRowCount(verticalHeaderLabels.count());
	setVerticalHeaderLabels(verticalHeaderLabels);
	verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);

	QHeaderView *headerView = new QHeaderView(Qt::Horizontal, this);
	headerView->setResizeMode(QHeaderView::Stretch);
	setHorizontalHeader(headerView);
	horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	horizontalHeader()->setStretchLastSection(true);
	
	resizeRowsToContents();
	resizeColumnsToContents();
	
	connect(this, SIGNAL(cellChanged(int,int)), this, SLOT(cellChangedSlot(int,int)));
}

IfacesTable::~IfacesTable()
{
	int row, col;
	for(row = 0; row < ifaces.size(); row++)
	{
		for(col = 0; col < columnAt(row); col++)
		{
			if(col == COLUMN_IFACE_ENABLED || col == COLUMN_MAC)
				delete cellWidget(row, col);
			else
				delete itemAt(row, col);
		}
	}

	while(!ifaces.empty())
	{
		delete ifaces.back();
		ifaces.pop_back();
	}
}

#ifdef CONFIGURABLE_IP
int IfacesTable::addIface(bool enabled, QString mac, QString name, QString ip, QString subnetMask, QString subnetName)
#else
int IfacesTable::addIface(bool enabled, QString mac, QString name, QString subnetName)
#endif
{
	disconnect(this, SIGNAL(cellChanged(int,int)), this, SLOT(cellChangedSlot(int,int)));

#ifdef CONFIGURABLE_IP
	Iface *i = new Iface(enabled, mac, name, ip, subnetMask, subnetName);
#else
	Iface *i = new Iface(enabled, mac, name, subnetName);
#endif

	ifaces.push_back(i);
	int row = ifaces.size() - 1;

	setCellWidget(row, COLUMN_IFACE_ENABLED, new IfaceEnableCheckBox(this, row, this));
	setCellWidget(row, COLUMN_MAC, new MacWidgetField(this, row, this));
	setItem(row, COLUMN_IFACE_NAME, new QTableWidgetItem());
#ifdef CONFIGURABLE_IP
	setItem(row, COLUMN_IP, new QTableWidgetItem());
	setItem(row, COLUMN_SUBNETMASK, new QTableWidgetItem());
#endif
	setItem(row, COLUMN_SUBNETNAME, new QTableWidgetItem());

	int col;
	for (col = 1; col < columnCount(); col++)
	{
		if(col != COLUMN_MAC)
			item(row, col)->setTextAlignment(Qt::AlignCenter);
		else
			((MacWidgetField *)cellWidget(row, col))->lineEdit->setAlignment(Qt::AlignCenter);
	}

	setStatus(row, enabled);
	setMac(row, i->mac);
	setName(row, i->name);
#ifdef CONFIGURABLE_IP
	setIp(row, i->ip);
	setSubnetMask(row, i->subnetMask);
#endif
	setSubnetName(row, i->subnetName);

	connect(this, SIGNAL(cellChanged(int,int)), this, SLOT(cellChangedSlot(int,int)));
	return row;
}

bool IfacesTable::setStatus(int iface, bool checked)
{
	((IfaceEnableCheckBox *)cellWidget(iface, COLUMN_IFACE_ENABLED))->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
	int col;
	for (col = 1; col < columnCount(); col++)
	{
		Qt::ItemFlags flags;
		if (col == COLUMN_MAC)
		{
			MacWidgetField *w = (MacWidgetField *)cellWidget(iface, col);
			w->button->setEnabled(checked);
			w->lineEdit->setEnabled(checked);
		}
		else
		{
			flags = item(iface, col)->flags();
			if (checked)
				flags |= Qt::ItemIsEnabled;
			else
				flags &= ~Qt::ItemIsEnabled;
			item(iface, col)->setFlags(flags);
		}
	}

	ifaces.at(iface)->enabled = checked;
	return true;
}


bool IfacesTable::setName(int iface, QString name)
{
	int i;
	for (i = 0; i < ifaces.size(); i++)
	{
		if (name == ifaces.at(i)->name)
		{
			item(iface, COLUMN_IFACE_NAME)->setText(ifaces.at(iface)->name);
			return false;
		}
	}
	
	ifaces.at(iface)->setName(name);
	QString new_name = ifaces.at(iface)->name;
	item(iface, COLUMN_IFACE_NAME)->setText(new_name);
	return true;
}

bool IfacesTable::setMac(int iface, QString mac)
{
	int i;
	for (i = 0; i < ifaces.size(); i++)
	{
		if (Iface::formatMac(mac) == ifaces.at(i)->mac)
		{
			((MacWidgetField *)cellWidget(iface, COLUMN_MAC))->setText(ifaces.at(iface)->mac);
			return false;
		}
	}
	
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

#ifdef CONFIGURABLE_IP
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
#endif

bool IfacesTable::setSubnetName(int iface, QString subnetName)
{
	ifaces.at(iface)->setSubnetName(subnetName);
	QString new_subnetName = ifaces.at(iface)->subnetName;
	item(iface, COLUMN_SUBNETNAME)->setText(new_subnetName);
	return true;
}

void IfacesTable::generateMac(int iface)
{
	setMac(iface, vboxbridge->generateMac());
}

QStringList IfacesTable::getIfaceInfo(int iface)
{
	QStringList iface_info;
	iface_info.push_back(ifaces.at(iface)->name);
	iface_info.push_back(ifaces.at(iface)->mac);
#ifdef CONFIGURABLE_IP
	iface_info.push_back(ifaces.at(iface)->ip);
	iface_info.push_back(ifaces.at(iface)->subnetMask);
#endif
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
		case COLUMN_MAC:
// 			setMac(row, item(row, COLUMN_MAC)->text());
			break;
		case COLUMN_IFACE_NAME:
			setName(row, item(row, COLUMN_IFACE_NAME)->text());
			break;
#ifdef CONFIGURABLE_IP
		case COLUMN_IP:
			setIp(row, item(row, COLUMN_IP)->text());
			break;
		case COLUMN_SUBNETMASK:
			setSubnetMask(row, item(row, COLUMN_SUBNETMASK)->text());
			break;
#endif
		case COLUMN_SUBNETNAME:
			setSubnetName(row, item(row, COLUMN_SUBNETNAME)->text());
			break;
	}
}
