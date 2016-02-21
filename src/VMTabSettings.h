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

#ifndef VMTABSETTINGS_H
#define VMTABSETTINGS_H

#include <vector>
#include <QWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include "IfacesTable.h"
#include "VirtualMachine.h"
#include "VirtualBoxBridge.h"
#include "SummaryDialog.h"
#include "VMSettings.h"

class MachinesDialog;
class MainWindow;

class VMTabSettings : public QWidget
{
	friend class MachinesDialog;
	friend class MainWindow;
	friend class IfacesTable;
	friend class AttachmentDataWidget;
	friend class SummaryDialog;

	Q_OBJECT
	
	public:
		VMTabSettings(QTabWidget *parent, QString tabname, VirtualBoxBridge *vboxbridge, MachineBridge *machine, std::string vhd_mountpoint, std::string partition_mountpoint_prefix);
		virtual ~VMTabSettings();
		IfacesTable *ifaces_table;
		void refreshTable();
		void refreshTableUI();
		void lockSettings();
		void unlockSettings();
		bool hasThisMachine(MachineBridge *_machine);
		QString getMachineName() const { return vm->machine->getName(); };
		bool setMachineName(const QString &qName);
		QString getMachineUUID() const { return vm->machine->getUUID(); };
		bool setMachineUUID(const char *uuid);
		VirtualMachine *vm;
		
	private:
		QCheckBox *vm_enabled;
		QWidget *vm_tab;
		QVBoxLayout *verticalLayout;
		QDialogButtonBox *buttonBox;
		Iface **ifaces;
		VirtualBoxBridge *vboxbridge;
		MachineBridge *machine;
		VMSettings *vmSettings;
		
	private slots:
		void clickedSlot(QAbstractButton*);
		void vm_enabledSlot(bool checked);
		void slotIfaceChange(int iface, ifacekey_t key, void *value_ptr);
};

#endif //VMTABSETTINGS_H
