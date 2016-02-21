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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QMainWindow>
#include <QFile>
#include <vector>

#include "ui_MainWindow.h"
#include "ui_info_dialog.h"
#include "InfoDialog.h"
#include "VMTabSettings.h"
#include "VirtualBoxBridge.h"
#include "SummaryDialog.h"
#include "VMSettings.h"
#include "MachinesDialog.h"

class Ui_MainWindow;
class Ui_Info_dialog;
class QFile;

class MainWindow : public QMainWindow
{
	friend class CloneDialog;
	friend class SummaryDialog;
	Q_OBJECT
	
	public:
		explicit MainWindow(const QString &fileToOpen = QString(), QWidget *parent = 0);
		~MainWindow();
		int launchCreateProcess(QString qName, bool reInitIfaces, bool restoreFromFile = false);
		void launchCloneProcess(QString qName, bool reInitIfaces);
		VirtualBoxBridge *vboxbridge;
		static QPalette getPalette();
		
	protected:
		void closeEvent(QCloseEvent *event);
	
	private slots:
		void slotInfo();
		void currentChangedSlot(int tab);
		void slotNew();
		void slotClone();
		void slotRemove();
		void slotRename();
		void slotStartAll();
		void slotInterrompiAll();
		void slotEnableAll();
		void slotDisableAll();
		void slotShowSummary();
		void slotStart();
		void slotPause();
		void slotReset();
		void slotStop();
// 		void slotSettings();
		void slotStateChange(MachineBridge *machine, uint32_t state);
		void slotNetworkAdapterChange(MachineBridge *machine, INetworkAdapter *nic);
		void slotVMLoad();
		bool slotVMSave();
		bool slotVMSaveAs();
		void slotImportMachines();
		void slotExportMachines();
		
	private:
		bool queryClose();
		bool loadFile(const QString &path);
		void setSettingsPolicy(int tab, uint32_t state);
		void refreshUI(int tab, uint32_t state = -1);
		VMTabSettings *addMachine(IMachine *m);
		
		Ui_MainWindow *ui;
		std::vector<VMTabSettings*> VMTabSettings_vec;
		std::vector<MachineBridge*> machines_vec;
		InfoDialog infoDialog;
		SummaryDialog *summaryDialog;
		QString fileName;
		bool requestedACPIstop;
		std::vector<VMSettings*> VMSettings_vec;

	signals:
		void machinesPoolChanged();
};

#endif //MAINWINDOW_H
