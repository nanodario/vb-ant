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

class MainWindow;

class VMTabSettings : public QWidget
{
	friend class MainWindow;
	
	Q_OBJECT
	
	public:
		VMTabSettings(QTabWidget *parent, QString _vm_name, VirtualBoxBridge *vboxbridge, MachineBridge *machine);
		virtual ~VMTabSettings();
		IfacesTable *ifaces_table;
		void refreshTable();
		void refreshTableUI();
		void lockSettings();
		void unlockSettings();
		bool hasThisMachine(MachineBridge *_machine);
		
	private:
		QString vm_name;
		QCheckBox *vm_enabled;
		QWidget *vm_tab;
		QVBoxLayout *verticalLayout;
		QDialogButtonBox *buttonBox;
		VirtualMachine *vm;
		Iface **ifaces;
		VirtualBoxBridge *vboxbridge;
		MachineBridge *machine;
		
	private slots:
		void clickedSlot(QAbstractButton*);
		void vm_enabledSlot(bool checked);
		void slotIfaceChange(int iface, ifacekey_t key, void *value_ptr);
};

#endif //VMTABSETTINGS_H
