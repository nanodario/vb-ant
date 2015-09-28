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
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "VMTabSettings.h"
#include "InfoDialog.h"
#include "VirtualBoxBridge.h"
#include "OSBridge.h"
#include "CloneDialog.h"
#include "ProgressDialog.h"

MainWindow::MainWindow(const QString &fileToOpen, QWidget *parent)
: QMainWindow(parent), ui(new Ui_MainWindow), vboxbridge(new VirtualBoxBridge()), machines_vec(vboxbridge->getMachines(this))
{
	VMTabSettings_vec.clear();
	ui->setupUi(this);

	if(OSBridge::checkNbdModule())
		OSBridge::unloadNbdModule();

	OSBridge::loadNbdModule(machines_vec.size());

	const char *tmpdir = getenv("TMPDIR");
	if(tmpdir == NULL)
		tmpdir = "/tmp";

	std::stringstream tmpdir_prefix; tmpdir_prefix << tmpdir << "/" << PROGRAM_NAME;

	struct stat s;
	if(stat(tmpdir_prefix.str().c_str(), &s) < 0 && errno == ENOENT)
		mkdir(tmpdir_prefix.str().c_str(), 0777);

	ProgressDialog p("");
	p.ui->label->setText("Caricamento impostazioni...");
	p.ui->progressBar->setValue(0);
	p.show();

	for (int i = 0; i < machines_vec.size(); i++)
	{
		QString tabname = machines_vec.at(i)->getName();
		p.ui->label->setText(QString::fromUtf8("Caricamento macchina \"").append(tabname).append("\""));

		std::stringstream mountpoint_ss; mountpoint_ss << "/dev/nbd" << i;
		std::stringstream partition_mountpoint_prefix_ss; partition_mountpoint_prefix_ss << tmpdir_prefix.str() << "/nbd" << i;

		VMTabSettings *vmSettings = new VMTabSettings(ui->vm_tabs, tabname, vboxbridge, machines_vec.at(i), mountpoint_ss.str(), partition_mountpoint_prefix_ss.str());
	
		ui->vm_tabs->addTab(vmSettings, tabname);
		VMTabSettings_vec.push_back(vmSettings);
		p.ui->progressBar->setValue(((i+1)*100)/machines_vec.size());
	}
	p.ui->label->setText("Caricamento completato");
	p.ui->progressBar->setValue(100);

	connect(ui->actionInfo_su, SIGNAL(triggered(bool)), this, SLOT(slotInfo()));
// 	connect(ui->actionNew, SIGNAL(triggered(bool)), this, SLOT(slotActionNew()));
// 	connect(ui->actionopen, SIGNAL(triggered(bool)), this, SLOT(slotActionOpen()));
// 	connect(ui->actionSave, SIGNAL(triggered(bool)), this, SLOT(slotActionSave()));
// 	connect(ui->actionSaveAs, SIGNAL(triggered(bool)), this, SLOT(slotActionSaveAs()));
	connect(ui->vm_tabs, SIGNAL(currentChanged(int)), this, SLOT(currentChangedSlot(int)));

	connect(ui->actionClona, SIGNAL(triggered(bool)), this, SLOT(slotClone()));
	connect(ui->actionElimina, SIGNAL(triggered(bool)), this, SLOT(slotRemove()));
	connect(ui->actionAvviaAll, SIGNAL(triggered(bool)), this, SLOT(slotStartAll()));
	connect(ui->actionInterrompiAll, SIGNAL(triggered(bool)), this, SLOT(slotInterrompiAll()));
	connect(ui->actionAvvia, SIGNAL(triggered(bool)), this, SLOT(slotStart()));
	connect(ui->actionPausa, SIGNAL(triggered(bool)), this, SLOT(slotPause()));
	connect(ui->actionReset, SIGNAL(triggered(bool)), this, SLOT(slotReset()));
	connect(ui->actionInterrompi, SIGNAL(triggered(bool)), this, SLOT(slotStop()));
// 	connect(ui->actionImpostazioni, SIGNAL(triggered(bool)), this, SLOT(slotSettings()));

	connect(ui->toolbarClona, SIGNAL(triggered(bool)), ui->actionClona, SIGNAL(triggered(bool)));
	connect(ui->toolbarElimina, SIGNAL(triggered(bool)), ui->actionElimina, SIGNAL(triggered(bool)));
	connect(ui->toolbarAvviaAll, SIGNAL(triggered(bool)), ui->actionAvviaAll, SIGNAL(triggered(bool)));
	connect(ui->toolbarInterrompiAll, SIGNAL(triggered(bool)), ui->actionInterrompiAll, SIGNAL(triggered(bool)));
	connect(ui->toolbarAvvia, SIGNAL(triggered(bool)), ui->actionAvvia, SIGNAL(triggered(bool)));
	connect(ui->toolbarPausa, SIGNAL(triggered(bool)), ui->actionPausa, SIGNAL(triggered(bool)));
	connect(ui->toolbarReset, SIGNAL(triggered(bool)), ui->actionReset, SIGNAL(triggered(bool)));
	connect(ui->toolbarInterrompi, SIGNAL(triggered(bool)), ui->actionInterrompi, SIGNAL(triggered(bool)));
// 	connect(ui->toolbarImpostazioni, SIGNAL(triggered(bool)), ui->actionImpostazioni, SIGNAL(triggered(bool)));

	ui->retranslateUi(this);
	setWindowTitle(QString::fromUtf8(PROGRAM_NAME).toUpper());
	ui->menuFile->setTitle(QString::fromUtf8(PROGRAM_NAME).toUpper());
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

	const char *tmpdir = getenv("TMPDIR");
	if(tmpdir == NULL)
		tmpdir = "/tmp";

	std::stringstream tmpdir_prefix; tmpdir_prefix << tmpdir << "/" << PROGRAM_NAME;

	struct stat s;
	if(stat(tmpdir_prefix.str().c_str(), &s) >= 0 && ((s.st_mode & S_IFMT) == S_IFDIR))
		rmdir(tmpdir_prefix.str().c_str());

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
	for(int i = 0; i < VMTabSettings_vec.size(); i++)
	{
		uint32_t machineState = VMTabSettings_vec.at(i)->machine->getState();

		if(machineState == MachineState::Running ||
			machineState == MachineState::Paused ||
			machineState == MachineState::Starting)
		{
			QMessageBox qm(QMessageBox::Warning, "Confermare uscita", "Alcune macchine virtuali sono ancora avviate.\nUscire comunque?", QMessageBox::Yes | QMessageBox::No, this);
			qm.setPalette(palette());
			for(int i = 0; i < qm.buttons().size(); i++)
			{
				switch(qm.standardButton(qm.buttons()[i]))
				{
					case QDialogButtonBox::Yes: qm.buttons()[i]->setText(QString::fromUtf8("Sì")); break;
					case QDialogButtonBox::No: qm.buttons()[i]->setText("No"); break;
				}
			}

			switch (qm.exec())
			{
				case QMessageBox::Yes:
					return true;
				case QMessageBox::No:
					return false;
			}
		}
	}

	return true;
}

#if 0
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
#endif

void MainWindow::slotInfo()
{
	infoDialog.show();
}

void MainWindow::currentChangedSlot(int tab)
{
	refreshUI(tab);
}

void MainWindow::slotStartAll()
{
	QMessageBox qm(QMessageBox::Question, "Avvio multiplo macchine", "Avviare tutte le macchine abilitate?", QMessageBox::Yes|QMessageBox::No, this);
	qm.setPalette(palette());
	for(int i = 0; i < qm.buttons().size(); i++)
	{
		switch(qm.standardButton(qm.buttons()[i]))
		{
			case QDialogButtonBox::Yes: qm.buttons()[i]->setText(QString::fromUtf8("Sì")); break;
			case QDialogButtonBox::No: qm.buttons()[i]->setText("No"); break;
		}
	}

	if (qm.exec() != QMessageBox::Yes)
		return;

	for(int i = 0; i < ui->vm_tabs->count(); i++)
	{
		if(VMTabSettings_vec.at(i)->vm_enabled->isChecked())
		{
			uint32_t machineState = VMTabSettings_vec.at(i)->machine->getState();

			if(machineState != MachineState::Running ||
				machineState != MachineState::Paused ||
				machineState != MachineState::Starting)
			{
				VMTabSettings_vec.at(i)->vm->start();

				uint32_t machineState = VMTabSettings_vec.at(i)->machine->getState();
				setSettingsPolicy(i, machineState);

				refreshUI(i);
			}
		}
	}
}

void MainWindow::slotInterrompiAll()
{
	QMessageBox qm(QMessageBox::Question, "Chiusura multipla macchine", "Arrestare tutte le macchine virtuali?", QMessageBox::Yes|QMessageBox::No, this);
	qm.setPalette(palette());
	for(int i = 0; i < qm.buttons().size(); i++)
	{
		switch(qm.standardButton(qm.buttons()[i]))
		{
			case QDialogButtonBox::Yes: qm.buttons()[i]->setText(QString::fromUtf8("Sì")); break;
			case QDialogButtonBox::No: qm.buttons()[i]->setText("No"); break;
		}
	}

	if (qm.exec() != QMessageBox::Yes)
		return;

	for(int i = 0; i < ui->vm_tabs->count(); i++)
	{
		uint32_t machineState = VMTabSettings_vec.at(i)->machine->getState();

		if(machineState == MachineState::Running ||
		   machineState == MachineState::Paused ||
		   machineState == MachineState::Starting)
			VMTabSettings_vec.at(i)->vm->stop();
	}
}

void MainWindow::slotStart()
{
	requestedACPIstop = false;
	VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->start();

	uint32_t machineState = VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->machine->getState();
	setSettingsPolicy(ui->vm_tabs->currentIndex(), machineState);

	refreshUI(ui->vm_tabs->currentIndex());
}

void MainWindow::slotPause()
{
	uint32_t machineState = ((VMTabSettings *)VMTabSettings_vec.at(ui->vm_tabs->currentIndex()))->machine->getState();

	if(machineState != MachineState::Paused)
		VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->enterPause();
	else
		VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->exitPause();
}

void MainWindow::slotReset()
{
	VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->reset();
}

void MainWindow::slotStop()
{
	uint32_t machineState = VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->machine->getState();
	bool acpiEnabled = false;

	bool wasPaused = (machineState == MachineState::Paused);
	if(wasPaused)
		slotPause();
	
	acpiEnabled = VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->machine->supportsACPI();
	slotPause();

	QMessageBox qm(QMessageBox::Question, "Chiudi la macchina virtuale", "Arrestare la macchina virtuale?", QMessageBox::Yes|QMessageBox::No, this);
	QCheckBox *c = new QCheckBox("Forza arresto", &qm);
	c->setGeometry(QRect(86, 40, 300, 21));
	c->setChecked(false);
	c->setPalette(palette());
	qm.setPalette(palette());

	if (!acpiEnabled)
	{
		c->setChecked(true);
		c->setEnabled(false);
	}

	if (qm.exec() == QMessageBox::Yes)
	{
		if(c->isChecked())
			VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->stop();
		else
		{
			machineState = VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->machine->getState();
			if(machineState == MachineState::Paused)
				slotPause();
			VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->ACPIstop();
			requestedACPIstop = true;
		}
	}
	else
	{
		machineState = VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->machine->getState();
		if(!wasPaused && machineState == MachineState::Paused)
			slotPause();
	}

	delete c;
}

#if 0
void MainWindow::slotSettings()
{
	VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->openSettings();
}
#endif

void MainWindow::slotClone()
{
	CloneDialog *c = new CloneDialog(this);
	c->exec();
}

void MainWindow::launchCloneProcess(QString qName, bool reInitIfaces)
{
	IMachine *m = VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->clone(qName, reInitIfaces);
	if(m == NULL)
		return;

	OSBridge::unloadNbdModule();
	OSBridge::loadNbdModule(machines_vec.size() + 1);

	ProgressDialog p("");
	p.ui->label->setText(QString::fromUtf8("Caricamento macchina \"").append(qName).append("\""));
	p.ui->progressBar->setValue(0);
	p.open();

	int newTabIndex = ui->vm_tabs->count();

	machines_vec.push_back(new MachineBridge(vboxbridge, m, this));

	const char *tmpdir = getenv("TMPDIR");
	if(tmpdir == NULL)
		tmpdir = "/tmp";
	
	std::stringstream tmpdir_prefix; tmpdir_prefix << tmpdir << "/" << PROGRAM_NAME;
	std::stringstream mountpoint_ss; mountpoint_ss << "/dev/nbd" << newTabIndex;
	std::stringstream partition_mountpoint_prefix_ss; partition_mountpoint_prefix_ss << tmpdir_prefix.str() << "/nbd" << newTabIndex;

	QString tabname = machines_vec.at(newTabIndex)->getName();
	VMTabSettings *vmSettings = new VMTabSettings(ui->vm_tabs, tabname, vboxbridge, machines_vec.at(newTabIndex), mountpoint_ss.str(), partition_mountpoint_prefix_ss.str());
	vmSettings->vm->cleanIfaces(VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->ifaces, VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->ifaces_size);
	vmSettings->vm->copyIfaces(VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->ifaces, VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->ifaces_size);
	vmSettings->vm->saveSettings();
	vmSettings->refreshTable();

	ui->vm_tabs->addTab(vmSettings, tabname);
	VMTabSettings_vec.push_back(vmSettings);
	p.ui->progressBar->setValue(100);
}

void MainWindow::slotRemove()
{
	QMessageBox qm(QMessageBox::Question, "Rimuovi la macchina virtuale", "Eliminare la macchina virtuale?", QMessageBox::Yes|QMessageBox::No, this);
	qm.setPalette(palette());
	for(int i = 0; i < qm.buttons().size(); i++)
	{
		switch(qm.standardButton(qm.buttons()[i]))
		{
			case QDialogButtonBox::Yes: qm.buttons()[i]->setText(QString::fromUtf8("Sì")); break;
			case QDialogButtonBox::No: qm.buttons()[i]->setText("No"); break;
		}
	}

	int tabIndex = ui->vm_tabs->currentIndex();

	if (qm.exec() == QMessageBox::Yes)
	{
		if(VMTabSettings_vec.at(tabIndex)->vm->remove())
		{
			std::vector<VMTabSettings*> VMTabSettings_vec_shadow;
			std::vector<MachineBridge*> machines_vec_shadow;
			for(int i = 0; i < VMTabSettings_vec.size(); i++)
				if(i != tabIndex)
				{
					VMTabSettings_vec_shadow.push_back(VMTabSettings_vec.at(i));
					machines_vec_shadow.push_back(machines_vec.at(i));
				}

			ui->vm_tabs->removeTab(tabIndex);
			VMTabSettings *v = VMTabSettings_vec.at(tabIndex);
			MachineBridge *mb = machines_vec.at(tabIndex);
			VMTabSettings_vec = VMTabSettings_vec_shadow;
			machines_vec = machines_vec_shadow;

			delete v;
			delete mb;
		}
	}
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
	{
		VMTabSettings_vec.at(tabIndex)->vm->shutdownVMProcess();
		requestedACPIstop = false;
	}

	setSettingsPolicy(tabIndex, state);
	refreshUI(tabIndex, state);
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

void MainWindow::refreshUI(int tab, uint32_t state)
{
	if(state == -1)
		state = ((VMTabSettings *)VMTabSettings_vec.at(tab))->machine->getState();

	switch(state)
	{
		case MachineState::Starting:
		case MachineState::Stopping:
		case MachineState::Running:
		{
			ui->actionClona->setEnabled(false);
			ui->actionElimina->setEnabled(false);
			ui->actionAvviaAll->setEnabled(true);
			ui->actionInterrompiAll->setEnabled(true);
			ui->actionAvvia->setEnabled(false);
			ui->actionPausa->setEnabled(true); ui->actionPausa->setChecked(false);
			ui->actionReset->setEnabled(true);
			ui->actionInterrompi->setEnabled(true);

			ui->toolbarClona->setEnabled(false);
			ui->toolbarElimina->setEnabled(false);
			ui->toolbarAvviaAll->setEnabled(true);
			ui->toolbarInterrompiAll->setEnabled(true);
			ui->toolbarAvvia->setEnabled(false);
			ui->toolbarPausa->setEnabled(true); ui->toolbarPausa->setChecked(false);
			ui->toolbarReset->setEnabled(true);
			ui->toolbarInterrompi->setEnabled(true);
			break;
		}
		case MachineState::Paused:
		{
			ui->actionClona->setEnabled(false);
			ui->actionElimina->setEnabled(false);
			ui->actionAvviaAll->setEnabled(true);
			ui->actionInterrompiAll->setEnabled(true);
			ui->actionAvvia->setEnabled(false);
			ui->actionPausa->setEnabled(true); ui->actionPausa->setChecked(true);
			ui->actionReset->setEnabled(false);
			ui->actionInterrompi->setEnabled(true);
			
			ui->toolbarClona->setEnabled(false);
			ui->toolbarElimina->setEnabled(false);
			ui->toolbarAvviaAll->setEnabled(true);
			ui->toolbarInterrompiAll->setEnabled(true);
			ui->toolbarAvvia->setEnabled(false);
			ui->toolbarPausa->setEnabled(true); ui->toolbarPausa->setChecked(true);
			ui->toolbarReset->setEnabled(false);
			ui->toolbarInterrompi->setEnabled(true);
			break;
		}
		case MachineState::PoweredOff:
		case MachineState::Null:
		{
			ui->actionClona->setEnabled(true);
			ui->actionElimina->setEnabled(true);
			ui->actionAvviaAll->setEnabled(true);
			ui->actionInterrompiAll->setEnabled(true);
			ui->actionAvvia->setEnabled(true);
			ui->actionPausa->setEnabled(false); ui->actionPausa->setChecked(false);
			ui->actionReset->setEnabled(false);
			ui->actionInterrompi->setEnabled(false);
			
			ui->toolbarClona->setEnabled(true);
			ui->toolbarElimina->setEnabled(true);
			ui->toolbarAvviaAll->setEnabled(true);
			ui->toolbarInterrompiAll->setEnabled(true);
			ui->toolbarAvvia->setEnabled(true);
			ui->toolbarPausa->setEnabled(false); ui->toolbarPausa->setChecked(false);
			ui->toolbarReset->setEnabled(false);
			ui->toolbarInterrompi->setEnabled(false);
			break;
		}
	}

	if(requestedACPIstop)
	{
		ui->actionPausa->setEnabled(false);
		ui->toolbarPausa->setEnabled(false);
	}
}
