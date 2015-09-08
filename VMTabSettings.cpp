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

VMTabSettings::VMTabSettings(QTabWidget *parent, QString tabname, VirtualBoxBridge *vboxbridge, MachineBridge *machine, std::string vhd_mountpoint, std::string partition_mountpoint_prefix) : QWidget(parent)
, vm_name(tabname), vboxbridge(vboxbridge), machine(machine), vm(new VirtualMachine(machine, vhd_mountpoint, partition_mountpoint_prefix))
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

	ifaces_table = new IfacesTable(this, verticalLayout, vboxbridge, machine, vm->getIfaces());
	verticalLayout->addWidget(ifaces_table);

	buttonBox = new QDialogButtonBox(this);
	buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
	buttonBox->setStandardButtons(QDialogButtonBox::Apply|QDialogButtonBox::Reset/*|QDialogButtonBox::Ok|QDialogButtonBox::Cancel*/);

	verticalLayout->addWidget(buttonBox);

	connect(vm_enabled, SIGNAL(toggled(bool)), this, SLOT(vm_enabledSlot(bool)));
	connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(clickedSlot(QAbstractButton*)));
// 	connect(vm->machine->session, SIGNAL(sigMachineStateChange()), this, SLOT(sltMachineStateChanged()));

	refreshTable();
	connect(ifaces_table, SIGNAL(sigIfaceChange(int, ifacekey_t, void*)), this, SLOT(slotIfaceChange(int, ifacekey_t, void*)));
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
	vm->mountVpartition(OS_PARTITION_NUMBER);
	for(int row = 0; row < ifaces_table->rowCount(); row++)
	{
		bool enabled = ifaces_table->operator[](row)->enabled;
		QString mac = ifaces_table->operator[](row)->mac;
		bool cableConnected = ifaces_table->operator[](row)->cableConnected;
		uint32_t attachmentType = ifaces_table->operator[](row)->attachmentType;
		QString attachmentData = ifaces_table->operator[](row)->attachmentData;
		QString name = ifaces_table->operator[](row)->name;
#ifdef CONFIGURABLE_IP
		QString ip = ifaces_table->operator[](row)->ip;
		QString subnetMask = ifaces_table->operator[](row)->subnetMask;
#endif
		
#ifdef CONFIGURABLE_IP
		ifaces_table->setIface(row, enabled, mac, cableConnected, attachmentType, attachmentData, name, ip, subnetMask);
#else
		ifaces_table->setIface(row, enabled, mac, cableConnected, attachmentType, attachmentData, name);
#endif
	}
	vm->umountVpartition(OS_PARTITION_NUMBER);
}

void VMTabSettings::refreshTableUI()
{
	ifaces_table->blockSignals(true);
	for(int row = 0; row < ifaces_table->rowCount(); row++)
	{
		ifaces_table->setIfaceEnabled(row, ifaces_table->operator[](row)->enabled);
		ifaces_table->setMac(row, ifaces_table->operator[](row)->mac);
		ifaces_table->setCableConnected(row, ifaces_table->operator[](row)->cableConnected);
		ifaces_table->setAttachmentType(row, ifaces_table->operator[](row)->attachmentType);
		ifaces_table->setAttachmentData(row, ifaces_table->operator[](row)->attachmentData);
		ifaces_table->setName(row, ifaces_table->operator[](row)->name);
#ifdef CONFIGURABLE_IP
		ifaces_table->setIp(row, ifaces_table->operator[](row)->ip);
		ifaces_table->setSubnetMask(row, ifaces_table->operator[](row)->subnetMask);
#endif
	}
	ifaces_table->blockSignals(false);
}

void VMTabSettings::clickedSlot(QAbstractButton *button)
{
	QDialogButtonBox::StandardButton standardButton = buttonBox->standardButton(button);
	switch(standardButton)
	{
		case QDialogButtonBox::Apply:
		{
			std::cout << "apply: \t" << vm_name.toStdString() << std::endl;
			uint32_t machineState = machine->getState();
			if(machineState == MachineState::Starting ||
			   machineState == MachineState::Running ||
			   machineState == MachineState::Paused)
				std::cout << "vm->saveSettings(): " << std::string(vm->saveSettingsRunTime() ? "true" : "false") << std::endl;
			else
				std::cout << "vm->saveSettings(): " << std::string(vm->saveSettings() ? "true" : "false") << std::endl;

			for (int i = 0; i < 8; i++)
			{
				QStringList iface_info = ifaces_table->getIfaceInfo(i);
				int j;
				for(j = 0; j < iface_info.size() - 1; j++)
					std::cout << iface_info.at(j).toStdString() << " \t";
				std::cout << iface_info.at(j).toStdString() << std::endl;
			}
			refreshTable();
			break;
		}
		case QDialogButtonBox::Reset:
			vm->populateIfaces();
			refreshTableUI();
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

void VMTabSettings::slotIfaceChange(int iface, ifacekey_t key, void *value_ptr)
{
	vm->setNetworkAdapterData(iface, key, value_ptr);
}

void VMTabSettings::lockSettings()
{
	ifaces_table->lockSettings();
// 	buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void VMTabSettings::unlockSettings()
{
	ifaces_table->unlockSettings();
// 	buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
}

bool VMTabSettings::hasThisMachine(MachineBridge *_machine)
{
	return machine == _machine;
}
