/*
 * VBANT - VirtualBox Advanced Network Tool
 * Copyright (C) 2015  Dario Messina
 *
 * This file is part of VBANT
 *
 * VBANT is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * VBANT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "IfacesTable.h"

// #include <Qt>
#include <QCheckBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QBoxLayout>
#include <QToolButton>
#include <QObject>
#include <QLineEdit>
#include <QComboBox>
#include <vector>

#include <iostream>
#include <malloc.h>
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
	button->setIcon(QIcon(QString::fromUtf8(":/refresh/resources/refresh_16px.png")));
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

CustomCheckBox::CustomCheckBox(QWidget *parent, int row, void* destination, bool (*function_forwarder)(void*, int, bool)) : QWidget(parent)
, row(row), function_forwarder(function_forwarder), destination(destination)
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

CustomCheckBox::~CustomCheckBox()
{
	disconnect(checkbox, SIGNAL(toggled(bool)), this, SLOT(toggledSlot(bool)));
	delete layout();
	delete checkbox;
}

void CustomCheckBox::setCheckState(Qt::CheckState checked)
{
	checkbox->setCheckState(checked);
}

void CustomCheckBox::toggledSlot(bool checked)
{
	function_forwarder(destination, row, checked);
}

AttachmentComboBox::AttachmentComboBox(QWidget *parent, int row, IfacesTable *destination) : QWidget(parent)
, row(row), destination(destination)
{
	QHBoxLayout *horizontalLayout = new QHBoxLayout(this);
	horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
	horizontalLayout->setContentsMargins(0, 0, 0, 0);

	uint32_t maxIndex = std::max(
		std::max(
			std::max((uint32_t) NetworkAttachmentType::Null, (uint32_t) NetworkAttachmentType::Bridged),
			 std::max((uint32_t) NetworkAttachmentType::Generic, (uint32_t) NetworkAttachmentType::Generic)
		),
		std::max(
			std::max((uint32_t) NetworkAttachmentType::HostOnly, (uint32_t) NetworkAttachmentType::Internal),
			 std::max((uint32_t) NetworkAttachmentType::NAT, (uint32_t) NetworkAttachmentType::NATNetwork)
		)
	);
	
	uint32_t minIndex = std::min(
		std::min(
			std::min((uint32_t) NetworkAttachmentType::Null, (uint32_t) NetworkAttachmentType::Bridged),
			 std::min((uint32_t) NetworkAttachmentType::Generic, (uint32_t) NetworkAttachmentType::Generic)
		),
		std::min(
			std::min((uint32_t) NetworkAttachmentType::HostOnly, (uint32_t) NetworkAttachmentType::Internal),
			 std::min((uint32_t) NetworkAttachmentType::NAT, (uint32_t) NetworkAttachmentType::NATNetwork)
		)
	);
	
	comboBox = new QComboBox(this);
	
	for(int i = minIndex; i <= maxIndex; i++)
	{
		switch(i)
		{
			case NetworkAttachmentType::Null: comboBox->addItem(QString::fromUtf8("Null")); break;
			case NetworkAttachmentType::Bridged: comboBox->addItem(QString::fromUtf8("Bridged")); break;
			case NetworkAttachmentType::Generic: comboBox->addItem(QString::fromUtf8("Generic")); break;
			case NetworkAttachmentType::HostOnly: comboBox->addItem(QString::fromUtf8("HostOnly")); break;
			case NetworkAttachmentType::Internal: comboBox->addItem(QString::fromUtf8("Internal")); break;
			case NetworkAttachmentType::NAT: comboBox->addItem(QString::fromUtf8("NAT")); break;
			case NetworkAttachmentType::NATNetwork: comboBox->addItem(QString::fromUtf8("NATNetwork")); break;
			default: std::cout << "NetworkAttachmentType::" << i << " is an unknown attachment type" << std::endl; break;
		}
	}
	
	horizontalLayout->addWidget(comboBox);
	horizontalLayout->setAlignment(Qt::AlignCenter);
	
	setLayout(horizontalLayout);
}

void AttachmentComboBox::add_connection()
{
	comboBox->connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChangedSlot(int)));
}

void AttachmentComboBox::remove_connection()
{
	comboBox->disconnect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChangedSlot(int)));
}

AttachmentComboBox::~AttachmentComboBox()
{
	remove_connection();
	delete comboBox;
}

void AttachmentComboBox::currentIndexChangedSlot(int index)
{
	destination->setAttachmentType(row, index);
}

bool setStatus_forwarder(void *context, int row, bool checked)
{
	return static_cast<IfacesTable*>(context)->setStatus(row, checked);
}

bool setCableConnected_forwarder(void *context, int row, bool checked)
{
	return static_cast<IfacesTable*>(context)->setCableConnected(row, checked);
}

IfacesTable::IfacesTable(QWidget *parent, QBoxLayout *layout, VirtualBoxBridge *vboxbridge, Iface **ifaces) : QTableWidget(parent)
, vboxbridge(vboxbridge), ifaces(ifaces)
{
	setObjectName(QString::fromUtf8("tableView"));

#ifdef CONFIGURABLE_IP
	QStringList horizontalHeaderLabels = QString("Abilita;Indirizzo MAC;Collegata;Nome;Indirizzo IP;Maschera sottorete;Tipo interfaccia;Nome sottorete").split(";");
#else
	QStringList horizontalHeaderLabels = QString("Abilita;Indirizzo MAC;Collegata;Nome;Tipo interfaccia;Nome sottorete").split(";");
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
	for(row = 0; row < rowCount(); row++)
	{
		for(col = 0; col < columnAt(row); col++)
		{
			if(col == COLUMN_IFACE_ENABLED)
				delete ((CustomCheckBox *)cellWidget(row, col));
			else if(col == COLUMN_MAC)
				delete ((MacWidgetField *)cellWidget(row, col));
			else if(col == COLUMN_IFACE_TYPE)
				delete ((AttachmentComboBox *)cellWidget(row, col));
			else
				delete itemAt(row, col);
		}
	}
}

#ifdef CONFIGURABLE_IP
int IfacesTable::setIface(int iface, bool enabled, QString mac, bool cableConnected, uint32_t attachmentType, QString subnetName, QString name, QString ip, QString subnetMask)
#else
int IfacesTable::setIface(int iface, bool enabled, QString mac, bool cableConnected, uint32_t attachmentType, QString subnetName, QString name)
#endif
{
	disconnect(this, SIGNAL(cellChanged(int,int)), this, SLOT(cellChangedSlot(int,int)));
	
	if(ifaces[iface] == NULL)
	{
#ifdef CONFIGURABLE_IP
		ifaces[iface] = new Iface(enabled, mac, cableConnected, attachmentType, subnetName, name, ip, subnetMask);
#else
		ifaces[iface] = new Iface(enabled, mac, cableConnected, attachmentType, subnetName, name);
#endif
	}
	else
	{
		ifaces[iface]->enabled = enabled;
		ifaces[iface]->setMac(mac);
		ifaces[iface]->cableConnected = cableConnected;
		ifaces[iface]->setAttachmentType(attachmentType);
		ifaces[iface]->setName(name);
#ifdef CONFIGURABLE_IP
		ifaces[iface]->setIp(ip);
		ifaces[iface]->setSubnetMask(subnetMask);
#endif
		ifaces[iface]->setSubnetName(subnetName);
	}

	setCellWidget(iface, COLUMN_IFACE_ENABLED, new CustomCheckBox(this, iface, this, &setStatus_forwarder));
	setCellWidget(iface, COLUMN_MAC, new MacWidgetField(this, iface, this));
	setCellWidget(iface, COLUMN_IFACE_CONNECTED, new CustomCheckBox(this, iface, this, &setCableConnected_forwarder));
	setItem(iface, COLUMN_IFACE_NAME, new QTableWidgetItem());
#ifdef CONFIGURABLE_IP
	setItem(iface, COLUMN_IP, new QTableWidgetItem());
	setItem(iface, COLUMN_SUBNETMASK, new QTableWidgetItem());
#endif
	AttachmentComboBox *attachmentComboBox = new AttachmentComboBox(this, iface, this);
	setCellWidget(iface, COLUMN_IFACE_TYPE, attachmentComboBox);
	setItem(iface, COLUMN_SUBNETNAME, new QTableWidgetItem());

	int col;
	for (col = 1; col < columnCount(); col++)
	{
		if(col == COLUMN_MAC)
			((MacWidgetField *)cellWidget(iface, col))->lineEdit->setAlignment(Qt::AlignCenter);
		else if(col != COLUMN_IFACE_TYPE && col != COLUMN_IFACE_CONNECTED)
			item(iface, col)->setTextAlignment(Qt::AlignCenter);
	}

	setStatus(iface, ifaces[iface]->enabled);
	setMac(iface, ifaces[iface]->mac);
	setCableConnected(iface, ifaces[iface]->cableConnected);
	setName(iface, ifaces[iface]->name);
#ifdef CONFIGURABLE_IP
	setIp(iface, ifaces[iface]->ip);
	setSubnetMask(iface, ifaces[iface]->subnetMask);
#endif
	setAttachmentType(iface, ifaces[iface]->attachmentType);
	setSubnetName(iface, ifaces[iface]->subnetName);
	
	connect(this, SIGNAL(cellChanged(int,int)), this, SLOT(cellChangedSlot(int,int)));
	attachmentComboBox->add_connection();

	return iface;
}

bool IfacesTable::setStatus(int iface, bool checked)
{
	((CustomCheckBox *)cellWidget(iface, COLUMN_IFACE_ENABLED))->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
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
		else if(col == COLUMN_IFACE_CONNECTED)
		{
			CustomCheckBox *c = (CustomCheckBox *)cellWidget(iface, col);
			c->checkbox->setEnabled(checked);
		}
		else if(col == COLUMN_IFACE_TYPE)
		{
			AttachmentComboBox *a = (AttachmentComboBox *)cellWidget(iface, col);
			a->setEnabled(checked);
		}
		else if(col == COLUMN_SUBNETNAME)
		{
			flags = item(iface, col)->flags();
			if(checked && ifaces[iface]->attachmentType != NetworkAttachmentType::Null)
				flags |= Qt::ItemIsEnabled;
			else
				flags &= ~Qt::ItemIsEnabled;
			item(iface, col)->setFlags(flags);
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

	ifaces[iface]->enabled = checked;
	return true;
}

bool IfacesTable::setName(int iface, QString name)
{
	int i;
	for (i = 0; i < rowCount(); i++)
	{
		if (name == ifaces[i]->name)
		{
			item(iface, COLUMN_IFACE_NAME)->setText(ifaces[iface]->name);
			return false;
		}
	}
	
	ifaces[iface]->setName(name);
	QString new_name = ifaces[iface]->name;
	item(iface, COLUMN_IFACE_NAME)->setText(new_name);
	return true;
}

bool IfacesTable::setMac(int iface, QString mac)
{
	int i;
	for (i = 0; i < rowCount(); i++)
	{
		if (Iface::formatMac(mac) == ifaces[i]->mac)
		{
			((MacWidgetField *)cellWidget(iface, COLUMN_MAC))->setText(ifaces[iface]->mac);
			return false;
		}
	}
	
	QString old_mac = ifaces[iface]->mac;
	bool done = ifaces[iface]->setMac(mac);
	if (done)
	{
		QString new_mac = ifaces[iface]->mac;
		((MacWidgetField *)cellWidget(iface, COLUMN_MAC))->setText(new_mac);
	}
	else
		((MacWidgetField *)cellWidget(iface, COLUMN_MAC))->setText(old_mac);
	
	return done;
}

bool IfacesTable::setCableConnected(int iface, bool checked) //TODO
{
	((CustomCheckBox *)cellWidget(iface, COLUMN_IFACE_CONNECTED))->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
	ifaces[iface]->cableConnected = checked;
	return true;
}

#ifdef CONFIGURABLE_IP
bool IfacesTable::setIp(int iface, QString ip)
{
	QString old_ip = ifaces[iface]->ip;
	bool done = ifaces[iface]->setIp(ip);
	if (done)
	{
		QString new_ip = ifaces[iface]->ip;
		item(iface, COLUMN_IP)->setText(ip);
	}
	else
		item(iface, COLUMN_IP)->setText(old_ip);

	return done;
}

bool IfacesTable::setSubnetMask(int iface, QString subnetMask)
{
	QString old_subnetMask = ifaces[iface]->subnetMask;
	bool done = ifaces[iface]->setSubnetMask(subnetMask);
	if (done)
	{
		QString new_subnetMask = ifaces[iface]->subnetMask;
		item(iface, COLUMN_SUBNETMASK)->setText(new_subnetMask);
	}
	else
		item(iface, COLUMN_SUBNETMASK)->setText(old_subnetMask);
	
	return done;
}
#endif

bool IfacesTable::setAttachmentType(int iface, uint32_t attachmentType) //FIXME
{
	AttachmentComboBox *attachmentComboBox = (AttachmentComboBox *)cellWidget(iface, COLUMN_IFACE_TYPE);

	if(ifaces[iface]->setAttachmentType(attachmentType))
	{
		attachmentComboBox->remove_connection();
		attachmentComboBox->comboBox->setCurrentIndex(ifaces[iface]->attachmentType);
		attachmentComboBox->add_connection();

		Qt::ItemFlags flags = item(iface, COLUMN_SUBNETNAME)->flags();
		if(ifaces[iface]->attachmentType != NetworkAttachmentType::Null)
			flags |= Qt::ItemIsEnabled;
		else
			flags &= ~Qt::ItemIsEnabled;
		item(iface, COLUMN_SUBNETNAME)->setFlags(flags);
		
		return true;
	}
	
	return false;
}

bool IfacesTable::setSubnetName(int iface, QString subnetName)
{
	ifaces[iface]->setSubnetName(subnetName);
	QString new_subnetName = ifaces[iface]->subnetName;
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
	iface_info.push_back(QString(ifaces[iface]->enabled ? "enabled" : "disabled"));
	iface_info.push_back(ifaces[iface]->mac);
	iface_info.push_back(QString(ifaces[iface]->cableConnected ? "connected" : "not connected"));
	iface_info.push_back(ifaces[iface]->name);
#ifdef CONFIGURABLE_IP
	iface_info.push_back(ifaces[iface]->ip);
	iface_info.push_back(ifaces[iface]->subnetMask);
#endif
	iface_info.push_back(QString("%1").arg(ifaces[iface]->attachmentType));
	iface_info.push_back(ifaces[iface]->subnetName);

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
		case COLUMN_IFACE_TYPE:
			break;
	}
}
