/*
 * VB-ANT - VirtualBox - Advanced Network Tool
 * Copyright (C) 2015, 2016  Dario Messina
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

#include "VMTabSettings.h"
#include "VirtualBoxBridge.h"
#include "VMSettings.h"
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
, vboxbridge(vboxbridge), machine(machine), vm(new VirtualMachine(machine, vhd_mountpoint, partition_mountpoint_prefix)), vmSettings(new VMSettings(vm))
{
	vm->vmSettings = vmSettings;

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

	for(int i = 0; i < buttonBox->buttons().size(); i++)
	{
		switch(buttonBox->standardButton(buttonBox->buttons()[i]))
		{
			case QDialogButtonBox::Apply: buttonBox->buttons()[i]->setText("Applica"); break;
			case QDialogButtonBox::Reset: buttonBox->buttons()[i]->setText("Reset"); break;
		}
	}

	verticalLayout->addWidget(buttonBox);

	connect(vm_enabled, SIGNAL(toggled(bool)), this, SLOT(vm_enabledSlot(bool)));
	connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(clickedSlot(QAbstractButton*)));

	refreshTable();
	connect(ifaces_table, SIGNAL(sigIfaceChange(int, ifacekey_t, void*)), this, SLOT(slotIfaceChange(int, ifacekey_t, void*)));
	connect(vm, SIGNAL(ifaceChanged(int)), ifaces_table, SLOT(slotRefreshIface(int)));
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
	vm->mountVpartition(OS_PARTITION_NUMBER, true);
	for(int row = 0; row < ifaces_table->rowCount(); row++)
	{
		vm->refreshIface(row, vm->machine->getIface(row));
		
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
		ifaces_table->setIfaceEnabled(row, ifaces[row]->enabled);
		ifaces_table->setMac(row, ifaces[row]->mac);
		ifaces_table->setCableConnected(row, ifaces[row]->cableConnected);
		ifaces_table->setAttachmentType(row, ifaces[row]->attachmentType);
		ifaces_table->setAttachmentData(row, ifaces[row]->attachmentData);
		ifaces_table->setName(row, ifaces[row]->name);
#ifdef CONFIGURABLE_IP
		ifaces_table->setIp(row, ifaces[row]->ip);
		ifaces_table->setSubnetMask(row, ifaces[row]->subnetMask);
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
			uint32_t machineState = machine->getState();
			if(machineState == MachineState::Starting ||
			   machineState == MachineState::Running ||
			   machineState == MachineState::Paused)
				std::cout << "vm->saveSettingsRuntime(): " << std::string(vm->saveSettingsRunTime() ? "true" : "false") << std::endl;
			else
				std::cout << "vm->saveSettings(): " << std::string(vm->saveSettings() ? "true" : "false") << std::endl;

/*
			for (int i = 0; i < 8; i++)
			{
				QStringList iface_info = ifaces_table->getIfaceInfo(i);
				int j;
				for(j = 0; j < iface_info.size() - 1; j++)
					std::cout << iface_info.at(j).toStdString() << " \t";
				std::cout << iface_info.at(j).toStdString() << std::endl;
			}
*/
			refreshTable();
			break;
		}
		case QDialogButtonBox::Reset:
// 			vm->populateIfaces();
// 			refreshTableUI();
			refreshTable();
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

bool VMTabSettings::setMachineName(const QString &qName)
{
	bool succeeded = true;

	if(!machine->lockMachine())
	{
		std::cerr << "[" << machine->getName().toStdString() <<  "] Cannot lock machine" << std::endl;
		return false;
	}

	if(!machine->setName(qName))
	{
		std::cout << "setName(" << qName.toStdString() << "): false" << std::endl;
		succeeded = false;
	}

	machine->saveSettings();

	if(!machine->unlockMachine())
		return false;

	return succeeded;
}

bool VMTabSettings::setMachineUUID(const char *uuid)
{
	bool succeeded = true;

	if(!machine->lockMachine())
	{
		std::cerr << "[" << machine->getName().toStdString() <<  "] Cannot lock machine" << std::endl;
		return false;
	}

	if(!machine->setUUID(uuid))
	{
		std::cout << "setUUID(" << uuid << "): false" << std::endl;
		succeeded = false;
	}

	machine->saveSettings();

	if(!machine->unlockMachine())
		return false;

	return succeeded;
}
