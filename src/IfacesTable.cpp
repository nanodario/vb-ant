/*
 * VB-ANT - VirtualBox - Advanced Network Tool
 * Copyright (C) 2015  Dario Messina
 *
 * This file is part of VB-ANT
 *
 * VB-ANT is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * VB-ANT is distributed in the hope that it will be useful,
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
#include "VirtualMachine.h"
#include "VMTabSettings.h"
#include "Iface.h"

#ifdef CONFIGURABLE_IP
	#define HORIZONTAL_HEADERS "Abilita;Indirizzo MAC;Collegata;Nome;Indirizzo IP;Maschera sottorete;Connessa a;Nome"
#else
	#define HORIZONTAL_HEADERS "Abilita;Indirizzo MAC;Collegata;Nome;Connessa a;Nome"
#endif

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
	button->setIcon(QIcon(QString::fromUtf8(":/refresh/resources/SVG PINO/ricollega.svg")));
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
			case NetworkAttachmentType::Null: comboBox->addItem(QString::fromUtf8("Non connesso")); break;
			case NetworkAttachmentType::Bridged: comboBox->addItem(QString::fromUtf8("Scheda con bridge")); break;
			case NetworkAttachmentType::Generic: comboBox->addItem(QString::fromUtf8("Driver generico")); break;
			case NetworkAttachmentType::HostOnly: comboBox->addItem(QString::fromUtf8("Scheda solo host")); break;
			case NetworkAttachmentType::Internal: comboBox->addItem(QString::fromUtf8("Rete interna")); break;
			case NetworkAttachmentType::NAT: comboBox->addItem(QString::fromUtf8("NAT")); break;
			case NetworkAttachmentType::NATNetwork: comboBox->addItem(QString::fromUtf8("Rete con NAT")); break;
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

AttachmentDataWidget::AttachmentDataWidget(QWidget *parent, int row, IfacesTable *destination) : QWidget(parent)
, comboBox(new QComboBox(this)), row(row), destination(destination)
{
	QHBoxLayout *horizontalLayout = new QHBoxLayout(this);
	horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
	horizontalLayout->setContentsMargins(0, 0, 0, 0);

	QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	setSizePolicy(sizePolicy);

	comboBox->setSizePolicy(sizePolicy);

	refreshWidget();

	horizontalLayout->addWidget(comboBox);
	horizontalLayout->setAlignment(Qt::AlignCenter);
	
	setLayout(horizontalLayout);
}

AttachmentDataWidget::~AttachmentDataWidget()
{
	remove_connection();
	delete comboBox;
}

void AttachmentDataWidget::add_connection()
{
	connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChangedSlot(int)));
}

void AttachmentDataWidget::remove_connection()
{
	disconnect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChangedSlot(int)));
}

void AttachmentDataWidget::refreshWidget()
{
	switch(destination->ifaces[row]->attachmentType)
	{
		case NetworkAttachmentType::NATNetwork:
		{
			std::vector<nsCOMPtr<INATNetwork> > natNetworks_vec = destination->vboxbridge->getNatNetworks();
			comboBox->clear();

			for(int i = 0; i < natNetworks_vec.size(); i++)
			{
				nsXPIDLString name;
				natNetworks_vec.at(i)->GetNetworkName(getter_Copies(name));
				comboBox->addItem(returnQStringValue(name));
			}
			break;
		}
		case NetworkAttachmentType::Bridged:
		{
			std::vector<nsCOMPtr<IHostNetworkInterface> > host_ifaces_vec = destination->vboxbridge->getHostNetworkInterfaces();
			comboBox->clear();

			for(int i = 0; i < host_ifaces_vec.size(); i++)
			{
				nsXPIDLString name;
				host_ifaces_vec.at(i)->GetName(getter_Copies(name));
				comboBox->addItem(returnQStringValue(name));
			}
			break;
		}
		case NetworkAttachmentType::Internal:
		{
			std::vector<QString> host_ifaces_vec = destination->vboxbridge->getInternalNetworkList();
			comboBox->clear();

			for(int i = 0; i < host_ifaces_vec.size(); i++)
				comboBox->addItem(host_ifaces_vec.at(i));
			break;
		}
		case NetworkAttachmentType::HostOnly:
		{
			std::vector<nsCOMPtr<IHostNetworkInterface> > hostOnly_ifaces_vec = destination->vboxbridge->getHostOnlyInterfaces();
			comboBox->clear();

			for(int i = 0; i < hostOnly_ifaces_vec.size(); i++)
			{
				nsXPIDLString name;
				((nsCOMPtr<IHostNetworkInterface>) hostOnly_ifaces_vec.at(i))->GetName(getter_Copies(name));
				comboBox->addItem(returnQStringValue(name));
			}
			break;
		}
		case NetworkAttachmentType::Generic:
		{
			std::vector<QString> generic_drivers_vec = destination->vboxbridge->getGenericDriversList();
			comboBox->clear();
			
			for(int i = 0; i < generic_drivers_vec.size(); i++)
				comboBox->addItem(generic_drivers_vec.at(i));
			break;
		}
		default:
			std::cout << "NetworkAttachmentType::" << destination->ifaces[row]->attachmentType << " is an unknown attachment type" << std::endl;
		case NetworkAttachmentType::Null:
		case NetworkAttachmentType::NAT:
		{
			comboBox->clear();
			break;
		}
		
	}
	refreshData();
	refreshStatus();
}

void AttachmentDataWidget::refreshStatus() //FIXME
{
	switch(destination->ifaces[row]->attachmentType)
	{
		case NetworkAttachmentType::NATNetwork:
		case NetworkAttachmentType::Bridged:
		case NetworkAttachmentType::HostOnly:
		{
			if(comboBox->isEditable())
				comboBox->lineEdit()->disconnect(comboBox->lineEdit(), SIGNAL(editingFinished()), this, SLOT(editingFinishedSlot()));
			comboBox->setEditable(false);
			break;
		}
		case NetworkAttachmentType::Internal:
		case NetworkAttachmentType::Generic:
		{
			comboBox->setEditable(true);
			comboBox->setAutoCompletion(true);
			comboBox->setAutoCompletionCaseSensitivity(Qt::CaseSensitive);
			comboBox->lineEdit()->connect(comboBox->lineEdit(), SIGNAL(editingFinished()), this, SLOT(editingFinishedSlot()));
			break;
		}
		default:
			std::cout << "NetworkAttachmentType::" << destination->ifaces[row]->attachmentType << " is an unknown attachment type" << std::endl;
		case NetworkAttachmentType::Null:
		case NetworkAttachmentType::NAT:
			setEnabled(false);
			return;
	}
	setEnabled(true);
}

void AttachmentDataWidget::refreshData()
{
	QString selected_entry;
	switch(destination->ifaces[row]->attachmentType)
	{
		case NetworkAttachmentType::NATNetwork: selected_entry = destination->machine->getNatNetwork(row); break;
		case NetworkAttachmentType::Bridged: selected_entry = destination->machine->getBridgedIface(row); break;
		case NetworkAttachmentType::Internal: selected_entry = destination->machine->getInternalName(row); break;
		case NetworkAttachmentType::HostOnly: selected_entry = destination->machine->getHostIface(row); break;
		case NetworkAttachmentType::Generic: selected_entry = destination->machine->getGenericDriver(row); break;
		default:
			std::cout << "NetworkAttachmentType::" << destination->ifaces[row]->attachmentType << " is an unknown attachment type" << std::endl;
		case NetworkAttachmentType::Null:
		case NetworkAttachmentType::NAT:
			return;
	}

	for(int i = 0; i < comboBox->count(); i++)
		if(selected_entry == comboBox->itemText(i))
			comboBox->setCurrentIndex(i);
}

void AttachmentDataWidget::currentIndexChangedSlot(int index)
{
	QString str = comboBox->currentText();
	destination->setAttachmentData(row, str);
}

void AttachmentDataWidget::editingFinishedSlot()
{
	QString str = comboBox->lineEdit()->text();
	destination->setAttachmentData(row, str);
}

IfacesTable::IfacesTable(QWidget *parent, QBoxLayout *layout, VirtualBoxBridge *vboxbridge, MachineBridge *machine, Iface **ifaces) : QTableWidget(parent)
, vboxbridge(vboxbridge), machine(machine), ifaces(ifaces)
{
	setObjectName(QString::fromUtf8("tableView"));

	QStringList horizontalHeaderLabels = QString(HORIZONTAL_HEADERS).split(";");

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
			else if(col == COLUMN_IFACE_TYPE_DATA)
				delete ((AttachmentDataWidget *)cellWidget(row, col));
			else
				delete itemAt(row, col);
		}
	}
}

#ifdef CONFIGURABLE_IP
int IfacesTable::setIface(int iface, bool enabled, QString mac, bool cableConnected, uint32_t attachmentType, QString attachmentData, QString name, QString ip, QString subnetMask)
#else
int IfacesTable::setIface(int iface, bool enabled, QString mac, bool cableConnected, uint32_t attachmentType, QString attachmentData, QString name)
#endif
{
	blockSignals(true);

	if(ifaces[iface] == NULL)
	{
#ifdef CONFIGURABLE_IP
		ifaces[iface] = new Iface(enabled, mac, cableConnected, attachmentType, attachmentData, name, ip, subnetMask);
#else
		ifaces[iface] = new Iface(enabled, mac, cableConnected, attachmentType, attachmentData, name);
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
		ifaces[iface]->setAttachmentData(attachmentData);
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
// 	CustomComboBox *customComboBox = new CustomComboBox(this, iface, this);
	AttachmentDataWidget *attachmentDataWidget = new AttachmentDataWidget(this, iface, this);
	setCellWidget(iface, COLUMN_IFACE_TYPE_DATA, attachmentDataWidget);

	int col;
	for (col = 1; col < columnCount(); col++)
	{
		if(col == COLUMN_MAC)
			((MacWidgetField *)cellWidget(iface, col))->lineEdit->setAlignment(Qt::AlignCenter);
		else if(col != COLUMN_IFACE_TYPE && col != COLUMN_IFACE_TYPE_DATA && col != COLUMN_IFACE_CONNECTED)
			item(iface, col)->setTextAlignment(Qt::AlignCenter);
	}

	setMac(iface, ifaces[iface]->mac);
	setCableConnected(iface, ifaces[iface]->cableConnected);
	setName(iface, ifaces[iface]->name);
#ifdef CONFIGURABLE_IP
	setIp(iface, ifaces[iface]->ip);
	setSubnetMask(iface, ifaces[iface]->subnetMask);
#endif
	setAttachmentType(iface, ifaces[iface]->attachmentType);
	setAttachmentData(iface, ifaces[iface]->attachmentData);
	setStatus(iface, ifaces[iface]->enabled);

	blockSignals(false);

	attachmentComboBox->add_connection();
	attachmentDataWidget->add_connection();

	return iface;
}

bool IfacesTable::setStatus(int iface, bool checked)
{
	CustomCheckBox *cCheckBox = ((CustomCheckBox *)cellWidget(iface, COLUMN_IFACE_ENABLED));
	cCheckBox->blockSignals(true);
	cCheckBox->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
	cCheckBox->blockSignals(false);
	
	for (int col = 1; col < columnCount(); col++)
	{
		Qt::ItemFlags flags;
		switch(col)
		{
			case COLUMN_MAC:
			{
				MacWidgetField *w = (MacWidgetField *)cellWidget(iface, col);
				w->button->setEnabled(checked);
				w->lineEdit->setEnabled(checked);
				break;
			}
			case COLUMN_IFACE_CONNECTED:
			{
				CustomCheckBox *c = (CustomCheckBox *)cellWidget(iface, col);
				c->checkbox->setEnabled(checked);
				break;
			}
			case COLUMN_IFACE_TYPE:
			{
				AttachmentComboBox *a = (AttachmentComboBox *)cellWidget(iface, col);
				a->setEnabled(checked);
				break;
			}
			case COLUMN_IFACE_TYPE_DATA:
			{
				AttachmentDataWidget *a = (AttachmentDataWidget *)cellWidget(iface, col);
				if(checked &&
					(ifaces[iface]->attachmentType != NetworkAttachmentType::Null
					&& ifaces[iface]->attachmentType != NetworkAttachmentType::NAT))
					a->setEnabled(true);
				else
					a->setEnabled(false);

				break;
			}
			default:
			{
				flags = item(iface, col)->flags();
				if (checked)
					flags |= Qt::ItemIsEnabled;
				else
					flags &= ~Qt::ItemIsEnabled;
				item(iface, col)->setFlags(flags);

				break;
			}
		}
	}

	ifaces[iface]->enabled = checked;
	return true;
}

bool IfacesTable::setIfaceEnabled(int iface, bool enabled)
{
	((CustomCheckBox *)cellWidget(iface, COLUMN_IFACE_ENABLED))->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
// 	emit sigIfaceChange(iface, IFACE_ENABLED, &enabled);
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
// 	emit sigIfaceChange(iface, IFACE_NAME, &new_name);
	
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
// 		emit sigIfaceChange(iface, IFACE_MAC, &new_mac);
	}
	else
		((MacWidgetField *)cellWidget(iface, COLUMN_MAC))->setText(old_mac);
	
	return done;
}

bool IfacesTable::setCableConnected(int iface, bool checked)
{
	((CustomCheckBox *)cellWidget(iface, COLUMN_IFACE_CONNECTED))->checkbox->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
	ifaces[iface]->cableConnected = checked;
	emit sigIfaceChange(iface, IFACE_CONNECTED, &checked);
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
		item(iface, COLUMN_IP)->setText(new_ip);
		if(ifaces[iface]->subnetMask.length() > 0)
		{
			if(ifaces[iface]->subnetMask.split(".").size() == 4)
				setSubnetMask(iface, QString::fromUtf8("%1").arg(Iface::subnetSizeFromSubnetMask(ifaces[iface]->subnetMask)));
			else
				setSubnetMask(iface, ifaces[iface]->subnetMask);
		}
// 		emit sigIfaceChange(iface, IFACE_IP, &new_ip);
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
// 		emit sigIfaceChange(iface, IFACE_SUBNETMASK, &new_subnetMask);
	}
	else
		item(iface, COLUMN_SUBNETMASK)->setText(old_subnetMask);
	
	return done;
}
#endif

bool IfacesTable::setAttachmentType(int iface, uint32_t attachmentType)
{
	AttachmentComboBox *attachmentComboBox = (AttachmentComboBox *)cellWidget(iface, COLUMN_IFACE_TYPE);
	AttachmentDataWidget *attachmentDataWidget = (AttachmentDataWidget *)cellWidget(iface, COLUMN_IFACE_TYPE_DATA);

	if(ifaces[iface]->setAttachmentType(attachmentType))
	{
		attachmentComboBox->remove_connection();
		attachmentDataWidget->remove_connection();
		attachmentComboBox->comboBox->setCurrentIndex(ifaces[iface]->attachmentType);
		attachmentDataWidget->refreshWidget();
		attachmentDataWidget->refreshStatus();
		attachmentDataWidget->add_connection();
		attachmentComboBox->add_connection();
		return true;
	}
	
	return false;
}

bool IfacesTable::setAttachmentData(int iface, QString attachmentData)
{
	ifaces[iface]->setAttachmentData(attachmentData);
	QString new_attachmentData = ifaces[iface]->attachmentData;	

	AttachmentDataWidget *attachmentDataWidget = (AttachmentDataWidget *)cellWidget(iface, COLUMN_IFACE_TYPE_DATA);
// 	AttachmentComboBox *attachmentComboBox = (AttachmentComboBox *)cellWidget(iface, COLUMN_IFACE_TYPE);
	if((ifaces[iface]->attachmentType == NetworkAttachmentType::Internal && vboxbridge->isNewInternalNetwork(attachmentData)) ||
		(ifaces[iface]->attachmentType == NetworkAttachmentType::Generic && vboxbridge->isNewGenericDriver(attachmentData)))
	{
		bool inserted = false;
		for(int i = 0; i < attachmentDataWidget->comboBox->count(); i++)
		{
			if(attachmentDataWidget->comboBox->itemText(i) == attachmentData)
			{
				inserted = true;
				break;
			}
		}

		if(!inserted)
		{
			attachmentDataWidget->comboBox->blockSignals(true);
			attachmentDataWidget->comboBox->addItem(attachmentData);
			attachmentDataWidget->comboBox->blockSignals(false);
		}
	}

// 	uint32_t attachmentType = attachmentComboBox->comboBox->currentIndex();
//
// 	if(attachmentType == NetworkAttachmentType::Internal ||
// 	   attachmentType == NetworkAttachmentType::Generic)
// 		((AttachmentDataWidget *)cellWidget(iface, COLUMN_IFACE_TYPE_DATA))->comboBox->lineEdit()->setText(new_attachmentData);
// 	emit sigIfaceChange(iface, IFACE_ATTACHMENT_DATA, &new_attachmentData);

	attachmentDataWidget->refreshStatus();
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
	iface_info.push_back(ifaces[iface]->attachmentData);

	return iface_info;
}

void IfacesTable::lockSettings()
{
	for(int iface = 0; iface < rowCount(); iface++)
	{
		for (int col = 0; col < columnCount(); col++)
		{
			Qt::ItemFlags flags;
			switch(col)
			{
				case COLUMN_IFACE_ENABLED:
				{
					CustomCheckBox *c = (CustomCheckBox *)cellWidget(iface, col);
					c->checkbox->setEnabled(false);
					break;
				}

				case COLUMN_MAC:
				{
					MacWidgetField *w = (MacWidgetField *)cellWidget(iface, col);
					w->button->setEnabled(false);
					w->lineEdit->setEnabled(false);
					break;
				}

				case COLUMN_IFACE_NAME:
#ifdef CONFIGURABLE_IP
				case COLUMN_IP:
				case COLUMN_SUBNETMASK:
#endif
				{
					flags = item(iface, col)->flags();
					flags &= ~Qt::ItemIsEnabled;
					item(iface, col)->setFlags(flags);
					break;
				}

				case COLUMN_IFACE_CONNECTED:
				case COLUMN_IFACE_TYPE:
				case COLUMN_IFACE_TYPE_DATA:
					break;
			}
		}
	}
}

void IfacesTable::unlockSettings()
{
	for(int iface = 0; iface < rowCount(); iface++)
	{
		((CustomCheckBox *)cellWidget(iface, COLUMN_IFACE_ENABLED))->checkbox->setEnabled(true);
		setStatus(iface, ((CustomCheckBox *)cellWidget(iface, COLUMN_IFACE_ENABLED))->checkbox->isChecked());
	}
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
		case COLUMN_IFACE_TYPE:
// 			setIfaceDataWidgetContent(row);
			break;
		case COLUMN_IFACE_TYPE_DATA:
// 			setSubnetName(row, item(row, COLUMN_SUBNETNAME)->text());
			break;
	}
}
