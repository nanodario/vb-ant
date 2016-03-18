/*
 * VB-ANT - VirtualBox - Advanced Network Tool
 * Copyright (C) 2016  Dario Messina
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

#ifndef MACHINESDIALOG_H
#define MACHINESDIALOG_H

#include <QDialog>
#include <QString>
#include <QCheckBox>
#include <vector>

#include "ui_MachinesDialog.h"
#include "VirtualMachine.h"
#include "VMSettings.h"

class MainWindow;
class VMTabSettings;
class Ui_MachinesDialog;

/**
 * Format for machines set save file:
 * magic bytes:
 * 	'S'					[   1 B]
 * 	number of machines			[   1 B]
 * 	machines data size			[   4 B]
 * 	SAVEFILE_MAGIC_BYTES			[variable lenght]
 * --- new line ---				[   1 B]
 * machine #1 data [variable lenght]:
 * 	machine data lenght			[   4 B]
 * 	header [2055 B]:
 * 		   machine name			[1024 B]
 * 		   machine uuid			[1024 B]
 * 		   ifaces checksum		[  10 B]
 * 		   number of ifaces		[   1 B]
 * 	ifaces #1 data (each iface) [5186 B]:
 * 		   last valid iface name	[1024 B]
 * 		   current iface name		[1024 B]
 * 		   iface mac			[  60 B]
 * 		   iface attachmentData		[1024 B]
 * 		   iface IP			[1024 B]
 * 		   iface subnet mask		[1024 B]
 * 		   iface attachment type	[   4 B]
 * 		   iface enabled flag		[   1 B]
 * 		   iface cable connected flag	[   1 B]
 * 	ifaces #2 data (each iface) [5186 B]
 * 	ifaces #n data (each iface) [5186 B]
 * machine #2 data [variable lenght]
 * machine #n data [variable lenght]
 */

class MachinesDialog : public QDialog
{
	Q_OBJECT;

	public:
		explicit MachinesDialog(MainWindow *mainwindow, std::vector<VMTabSettings*> *vmTab_vec, QString fileName = "");
		explicit MachinesDialog(MainWindow *mainwindow, std::vector<VMTabSettings*> *vmTab_vec, QPalette palette, QString fileName = "");
#ifndef EXAM_MODE
		bool buildDialog();
#else
		bool buildDialog(bool examMode = false);
#endif
		virtual ~MachinesDialog();

	private slots:
		void slotExportMachines();
		void slotImportMachines();
#ifdef EXAM_MODE
		void slotExamExportMachines();
#endif

	private:
#ifndef EXAM_MODE
		bool saveMachines(std::vector<VirtualMachine*> vm_vec, bool deflate);
#else
		bool saveMachines(std::vector<VirtualMachine*> vm_vec, bool deflate, bool examMode = false);
#endif
		read_result_t loadMachines(settings_header_t **settings_header, settings_iface_t ***settings_ifaces, uint32_t *machines_number);
		bool updateMachine(VMTabSettings *vmtab, settings_header_t settings_header, settings_iface_t *settings_ifaces);
		bool createMachine(settings_header_t settings_header, settings_iface_t *settings_ifaces);
		
		MainWindow *mainwindow;
		Ui_MachinesDialog *ui;
		std::vector<VMTabSettings*> *vmTab_vec;
		QString fileName;
		QCheckBox *checkBox;

		settings_header_t *settings_header;
		settings_iface_t **settings_ifaces;
		uint32_t machines_number;
};

#endif //MACHINESDIALOG_H
