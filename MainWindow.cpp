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

#include "MainWindow.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <sstream>
#include <iostream>
#include <bitset>
#include "stdint.h"

#include "VMTabSettings.h"
#include "InfoDialog.h"
#include "VirtualBoxBridge.h"

MainWindow::MainWindow(const QString &fileToOpen, QWidget *parent)
: QMainWindow(parent), ui(new Ui_MainWindow), vboxbridge(new VirtualBoxBridge()), machines_vec(vboxbridge->getMachines(this))
{
	VMTabSettings_vec.clear();
	ui->setupUi(this);

	int i;
	for (i = 0; i < machines_vec.size(); i++)
	{
		QString tabname = machines_vec.at(i)->getName();
		VMTabSettings *vmSettings = new VMTabSettings(ui->vm_tabs, tabname, vboxbridge, machines_vec.at(i));
		ui->vm_tabs->addTab(vmSettings, tabname);
		VMTabSettings_vec.push_back(vmSettings);
	}

	connect(ui->actionInfo_su, SIGNAL(triggered(bool)), this, SLOT(slotInfo()));
	connect(ui->actionNew, SIGNAL(triggered(bool)), this, SLOT(slotActionNew()));
	connect(ui->actionopen, SIGNAL(triggered(bool)), this, SLOT(slotActionOpen()));
	connect(ui->actionSave, SIGNAL(triggered(bool)), this, SLOT(slotActionSave()));
	connect(ui->actionSaveAs, SIGNAL(triggered(bool)), this, SLOT(slotActionSaveAs()));
	connect(ui->vm_tabs, SIGNAL(currentChanged(int)), this, SLOT(currentChangedSlot(int)));
	connect(ui->actionAvvia, SIGNAL(triggered(bool)), this, SLOT(slotStart()));
	connect(ui->actionPausa, SIGNAL(triggered(bool)), this, SLOT(slotPause()));
	connect(ui->actionReset, SIGNAL(triggered(bool)), this, SLOT(slotReset()));
	connect(ui->actionInterrompi, SIGNAL(triggered(bool)), this, SLOT(slotStop()));
	connect(ui->actionImpostazioni, SIGNAL(triggered(bool)), this, SLOT(slotSettings()));
	
	connect(ui->toolbarAvvia, SIGNAL(triggered(bool)), ui->actionAvvia, SIGNAL(triggered(bool)));
	connect(ui->toolbarPausa, SIGNAL(triggered(bool)), ui->actionPausa, SIGNAL(triggered(bool)));
	connect(ui->toolbarImpostazioni, SIGNAL(triggered(bool)), ui->actionImpostazioni, SIGNAL(triggered(bool)));
}

MainWindow::~MainWindow()
{
	disconnect(ui->vm_tabs, SIGNAL(currentChanged(int)), this, SLOT(currentChangedSlot(int)));
	while(!VMTabSettings_vec.empty())
	{
		delete VMTabSettings_vec.back();
		VMTabSettings_vec.pop_back();
	}

	while(!machines_vec.empty())
	{
		delete machines_vec.back();
		machines_vec.pop_back();
	}

	delete vboxbridge;
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (!queryClose())
		event->ignore();
}

bool MainWindow::queryClose()
{
/*
	// Se non ci sono cambiamenti non salvati
	if (!ui->actionSave->isEnabled())
		return true;

	switch (QMessageBox::warning(this, "Chiudi documento",
		"Sono presenti modifiche non salvate.\nSalvare i cambiamenti?",
		QMessageBox::Discard | QMessageBox::Save | QMessageBox::Cancel))
	{
		case QMessageBox::Discard:
			return true;
		case QMessageBox::Save:
			return slotSave();
		case QMessageBox::Cancel:
			return false;
	}
*/
	return true;
}

void MainWindow::slotActionNew()
{
	std::cout << "[" << __func__ << "]" << std::endl;
	
	/*
	const QString selectedFileName = QFileDialog::getOpenFileName(this,
		"Apri documento", fileName, "Grafo (*.graph)");
 
	if (selectedFileName.isEmpty() || !queryClose())
		return;
 
	loadFile(selectedFileName);
	*/
}

void MainWindow::slotActionOpen()
{
	std::cout << "[" << __func__ << "]" << std::endl;
		
	/*
	const QString selectedFileName = QFileDialog::getOpenFileName(this,
		"Apri documento", fileName, "Grafo (*.graph)");

	if (selectedFileName.isEmpty() || !queryClose())
		return;

	loadFile(selectedFileName);
	*/
}

bool MainWindow::slotActionSave()
{
	std::cout << "[" << __func__ << "]" << std::endl;
	
	if (fileName.isEmpty())
		return slotActionSaveAs();

	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
	{
		QMessageBox::critical(this, "Errore di salvataggio",
			"Impossibile scrivere sul file " + fileName);
		return false;
	}

	QTextStream(&file) << "test";
// 	ui->campo->setCleanUndoHistory();
	file.close();

	return true;
}

bool MainWindow::slotActionSaveAs()
{
	std::cout << "[" << __func__ << "]" << std::endl;

	const QString selectedFileName = QFileDialog::getSaveFileName(this, "Salva documento", fileName, "Grafo (*.graph)");

	if (selectedFileName.isEmpty())
		return false;

	fileName = selectedFileName;
	return slotActionSave();
// 	return true;
}

void MainWindow::slotInfo()
{
	infoDialog.show();
}

void MainWindow::currentChangedSlot(int tab)
{
	refreshUI(tab);
}

void MainWindow::slotStart()
{
// 	VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->lockSettings();
	VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->start();

	uint32_t machineState = VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->machine->getState();
	setSettingsPolicy(ui->vm_tabs->currentIndex(), machineState);

	refreshUI(ui->vm_tabs->currentIndex());
}

void MainWindow::slotPause()
{
	uint32_t machineState = ((VMTabSettings *)VMTabSettings_vec.at(ui->vm_tabs->currentIndex()))->machine->getState();
	if(machineState == MachineState::Paused)
		VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->exitPause();
	else
		VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->enterPause();
}

void MainWindow::slotReset()
{
	VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->reset();
}

void MainWindow::slotStop()
{
	uint32_t machineState = ((VMTabSettings *)VMTabSettings_vec.at(ui->vm_tabs->currentIndex()))->machine->getState();

	if(machineState != MachineState::Paused)
		slotPause();

	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, "Chiudi la macchina virtuale", "Arrestare la macchina virtuale?", QMessageBox::Yes|QMessageBox::No);

	if (reply == QMessageBox::Yes)
		VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->stop();
	else
	{
		machineState = ((VMTabSettings *)VMTabSettings_vec.at(ui->vm_tabs->currentIndex()))->machine->getState();
		if(machineState == MachineState::Paused)
			slotPause();
	}
}

void MainWindow::slotSettings()
{
	VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->openSettings();
}

void MainWindow::slotStateChange(MachineBridge *machine, uint32_t state)
{
	int tabIndex;
	for(int i = 0; i < VMTabSettings_vec.size(); i++)
		if(VMTabSettings_vec.at(i)->hasThisMachine(machine))
		{
			tabIndex = i;
			break;
		}

	if(state == MachineState::PoweredOff)
		VMTabSettings_vec.at(tabIndex)->vm->shutdownVMProcess();

	setSettingsPolicy(tabIndex, state);
	refreshUI(tabIndex);
}

void MainWindow::setSettingsPolicy(int tab, uint32_t state)
{
	switch(state)
	{
		case MachineState::Starting:
		case MachineState::Running:
		case MachineState::Paused:
			VMTabSettings_vec.at(tab)->lockSettings();
			break;
			
		case MachineState::PoweredOff:
			VMTabSettings_vec.at(tab)->unlockSettings();
			break;
			
		default: break;
	}
}

void MainWindow::slotNetworkAdapterChange(MachineBridge *machine, INetworkAdapter *nic)
{
	int tabIndex;
	for(int i = 0; i < VMTabSettings_vec.size(); i++)
		if(VMTabSettings_vec.at(i)->hasThisMachine(machine))
		{
			tabIndex = i;
			break;
		}

	VMTabSettings_vec.at(tabIndex)->vm->refreshIface(nic);
	VMTabSettings_vec.at(tabIndex)->refreshTableUI();

	setSettingsPolicy(tabIndex, VMTabSettings_vec.at(tabIndex)->machine->getState());
	refreshUI(tabIndex);
}

void MainWindow::refreshUI(int tab)
{
	uint32_t machineState = ((VMTabSettings *)VMTabSettings_vec.at(tab))->machine->getState();

	switch(machineState)
	{
		case MachineState::Starting:
		case MachineState::Running:
		{
			ui->actionElimina->setEnabled(false);
			ui->actionAvvia->setEnabled(false);
			ui->actionPausa->setEnabled(true);
			ui->actionPausa->setChecked(false);
			ui->actionReset->setEnabled(true);
			ui->actionInterrompi->setEnabled(true);

			ui->toolbarAvvia->setEnabled(false);
			ui->toolbarPausa->setEnabled(true);
			ui->toolbarPausa->setChecked(false);
			break;
		}

		case MachineState::Paused:
		{
			ui->actionElimina->setEnabled(false);
			ui->actionAvvia->setEnabled(false);
			ui->actionPausa->setEnabled(true);
			ui->actionPausa->setChecked(true);
			ui->actionReset->setEnabled(false);
			ui->actionInterrompi->setEnabled(true);

			ui->toolbarAvvia->setEnabled(false);
			ui->toolbarPausa->setEnabled(true);
			ui->toolbarPausa->setChecked(true);
			break;
		}

		case MachineState::PoweredOff:
		default:
		{
			ui->actionElimina->setEnabled(true);
			ui->actionAvvia->setEnabled(true);
			ui->actionPausa->setEnabled(false);
			ui->actionPausa->setChecked(false);
			ui->actionReset->setEnabled(false);
			ui->actionInterrompi->setEnabled(false);

			ui->toolbarAvvia->setEnabled(true);
			ui->toolbarPausa->setEnabled(false);
			ui->toolbarPausa->setChecked(false);
			break;
		}
	}
}
