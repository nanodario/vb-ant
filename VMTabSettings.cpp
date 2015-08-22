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

#include "VMTabSettings.h"
#include "VirtualBoxBridge.h"
#include <QTabWidget>

#include <QString>
#include <QStringList>
#include <QWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QHeaderView>

#include <iostream>

VMTabSettings::VMTabSettings(QTabWidget *parent, QString tabname, VirtualBoxBridge *vboxbridge, MachineBridge *machine) : QWidget(parent)
, vm_name(tabname), vboxbridge(vboxbridge), machine(machine), vm(new VirtualMachine(machine))
{
	setObjectName(tabname);

	verticalLayout = new QVBoxLayout(this);
	verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));

	vm_enabled = new QCheckBox(this);
	vm_enabled->setText(QString::fromUtf8("Abilita macchina virtuale"));
	vm_enabled->setChecked(true);
	vm_enabled->setEnabled(true);
	vm_enabled->setObjectName(QString::fromUtf8("vm_enabled"));
	verticalLayout->addWidget(vm_enabled);

	ifaces_table = new IfacesTable(this, verticalLayout, vboxbridge, vm->getIfaces());
	verticalLayout->addWidget(ifaces_table);

	buttonBox = new QDialogButtonBox(this);
	buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
	buttonBox->setStandardButtons(QDialogButtonBox::Apply|QDialogButtonBox::Reset/*|QDialogButtonBox::Ok|QDialogButtonBox::Cancel*/);

	verticalLayout->addWidget(buttonBox);

	connect(vm_enabled, SIGNAL(toggled(bool)), this, SLOT(vm_enabledSlot(bool)));
	connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(clickedSlot(QAbstractButton*)));
// 	connect(vm->machine->session, SIGNAL(sigMachineStateChange()), this, SLOT(sltMachineStateChanged()));

	refreshTable();
}

VMTabSettings::~VMTabSettings()
{
	delete buttonBox;
	delete vm;

	delete ifaces_table;
	delete vm_enabled;
	delete verticalLayout;
}

void VMTabSettings::refreshTable()
{
// 	ifaces = vm->getIfaces();

	for(int row = 0; row < ifaces_table->rowCount(); row++)
	{
		bool enabled = ifaces_table->operator[](row)->enabled;
		QString name = QString("test%1").arg(row);
		QString mac = ifaces_table->operator[](row)->mac;
		bool cableConnected = ifaces_table->operator[](row)->cableConnected;
		uint32_t attachmentType = ifaces_table->operator[](row)->attachmentType;
#ifdef CONFIGURABLE_IP
		QString ip = vm->getIp(row); //QString("10.10.10.%1").arg(row);
		QString subnetMask = vm->getSubnetMask(row); //QString("%1").arg(row);
#endif
		QString subnetName = ifaces_table->operator[](row)->subnetName;
		
#ifdef CONFIGURABLE_IP
		ifaces_table->setIface(row, enabled, mac, cableConnected, attachmentType, subnetName, name, ip, subnetMask);
#else
		ifaces_table->setIface(row, enabled, mac, cableConnected, attachmentType, subnetName, name);
#endif
	}
}

void VMTabSettings::clickedSlot(QAbstractButton *button)
{
	QDialogButtonBox::StandardButton standardButton = buttonBox->standardButton(button);
	switch(standardButton)
	{
		case QDialogButtonBox::Apply:
			std::cout << "apply: \t" << vm_name.toStdString() << std::endl;
			std::cout << "vm->saveSettings(): " << std::string(vm->saveSettings() ? "true" : "false") << std::endl;
			for (int i = 0; i < 8; i++)
			{
				QStringList iface_info = ifaces_table->getIfaceInfo(i);
				int j;
				for(j = 0; j < iface_info.size() - 1; j++)
					std::cout << iface_info.at(j).toStdString() << " \t";
				std::cout << iface_info.at(j).toStdString() << std::endl;
				
			}
			break;
		case QDialogButtonBox::Reset:
			std::cout << "reset: \t" << vm_name.toStdString() << std::endl;
			break;
		case QDialogButtonBox::Ok:
			std::cout << "ok: \t" << vm_name.toStdString() << std::endl;
			vm->start();
			break;
		case QDialogButtonBox::Cancel:
			std::cout << "cancel: \t" << vm_name.toStdString() << std::endl;
			vm->ACPIstop();
			break;
	}
}

void VMTabSettings::vm_enabledSlot(bool checked)
{
	ifaces_table->setDisabled(!checked);
}

void VMTabSettings::lockSettings()
{
	ifaces_table->lockSettings();
	buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void VMTabSettings::unlockSettings()
{
	ifaces_table->unlockSettings();
	buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
}

bool VMTabSettings::hasThisMachine(MachineBridge *_machine)
{
	return machine == _machine;
}
