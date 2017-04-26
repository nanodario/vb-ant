/*
 * VB-ANT - VirtualBox - Advanced Network Tool
 * Copyright (C) 2015 - 2017  Dario Messina
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

#include "MainWindow.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
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
#include "SummaryDialog.h"
#include "MachinesDialog.h"

static QPalette __palette;

QPalette MainWindow::getPalette()
{
	return __palette;
}

MainWindow::MainWindow(const QString &fileToOpen, QWidget *parent)
: QMainWindow(parent), ui(new Ui_MainWindow), vboxbridge(new VirtualBoxBridge())
, machines_vec(vboxbridge->getMachines(this))
{
	VMTabSettings_vec.clear();
	ui->setupUi(this);
	
	__palette = palette();

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
	
	OSBridge::cleanEnvironment(tmpdir_prefix.str());

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

		VMTabSettings *vmTabSettings = new VMTabSettings(ui->vm_tabs, tabname, vboxbridge, machines_vec.at(i), mountpoint_ss.str(), partition_mountpoint_prefix_ss.str());

		ui->vm_tabs->addTab(vmTabSettings, tabname);
		VMTabSettings_vec.push_back(vmTabSettings);
		p.ui->progressBar->setValue(((i+1)*100)/machines_vec.size());
		p.refresh();
	}
	p.ui->label->setText("Caricamento completato");
	p.ui->progressBar->setValue(100);

	summaryDialog = new SummaryDialog(this);

	for (int i = 0; i < VMTabSettings_vec.size(); i++)
	{
		connect(VMTabSettings_vec.at(i)->vm, SIGNAL(settingsChanged(VirtualMachine*)), summaryDialog, SLOT(refresh()));
		connect(this, SIGNAL(machinesPoolChanged()), summaryDialog, SLOT(populateComboBox()));
	}

	connect(ui->actionInfo_su, SIGNAL(triggered(bool)), this, SLOT(slotInfo()));
// 	connect(ui->actionopen, SIGNAL(triggered(bool)), this, SLOT(slotActionOpen()));
// 	connect(ui->actionSave, SIGNAL(triggered(bool)), this, SLOT(slotActionSave()));
// 	connect(ui->actionSaveAs, SIGNAL(triggered(bool)), this, SLOT(slotActionSaveAs()));
	connect(ui->vm_tabs, SIGNAL(currentChanged(int)), this, SLOT(currentChangedSlot(int)));

	connect(ui->actionNuova, SIGNAL(triggered(bool)), this, SLOT(slotNew()));
	connect(ui->actionClona, SIGNAL(triggered(bool)), this, SLOT(slotClone()));
	connect(ui->actionElimina, SIGNAL(triggered(bool)), this, SLOT(slotRemove()));
	connect(ui->actionRinomina, SIGNAL(triggered(bool)), this, SLOT(slotRename()));
	connect(ui->actionAvviaAll, SIGNAL(triggered(bool)), this, SLOT(slotStartAll()));
	connect(ui->actionInterrompiAll, SIGNAL(triggered(bool)), this, SLOT(slotInterrompiAll()));
	connect(ui->actionAbilitaAll, SIGNAL(triggered(bool)), this, SLOT(slotEnableAll()));
	connect(ui->actionDisabilitaAll, SIGNAL(triggered(bool)), this, SLOT(slotDisableAll()));
	connect(ui->actionMostraRiepilogo, SIGNAL(triggered(bool)), this, SLOT(slotShowSummary()));
	connect(ui->actionAvvia, SIGNAL(triggered(bool)), this, SLOT(slotStart()));
	connect(ui->actionPausa, SIGNAL(triggered(bool)), this, SLOT(slotPause()));
	connect(ui->actionReset, SIGNAL(triggered(bool)), this, SLOT(slotReset()));
	connect(ui->actionInterrompi, SIGNAL(triggered(bool)), this, SLOT(slotStop()));

	connect(ui->toolbarNuova, SIGNAL(triggered(bool)), ui->actionNuova, SIGNAL(triggered(bool)));
	connect(ui->toolbarClona, SIGNAL(triggered(bool)), ui->actionClona, SIGNAL(triggered(bool)));
	connect(ui->toolbarElimina, SIGNAL(triggered(bool)), ui->actionElimina, SIGNAL(triggered(bool)));
	connect(ui->toolbarAvviaAll, SIGNAL(triggered(bool)), ui->actionAvviaAll, SIGNAL(triggered(bool)));
	connect(ui->toolbarInterrompiAll, SIGNAL(triggered(bool)), ui->actionInterrompiAll, SIGNAL(triggered(bool)));
	connect(ui->toolbarAvvia, SIGNAL(triggered(bool)), ui->actionAvvia, SIGNAL(triggered(bool)));
	connect(ui->toolbarPausa, SIGNAL(triggered(bool)), ui->actionPausa, SIGNAL(triggered(bool)));
	connect(ui->toolbarReset, SIGNAL(triggered(bool)), ui->actionReset, SIGNAL(triggered(bool)));
	connect(ui->toolbarInterrompi, SIGNAL(triggered(bool)), ui->actionInterrompi, SIGNAL(triggered(bool)));

#ifdef EXAM_MODE
	ui->actionVMLoad->setDisabled(true);
	ui->actionVMSave->setDisabled(true);
	ui->actionVMSaveAs->setDisabled(true);
// 	ui->actionImportMachines->setDisabled(true);
// 	ui->actionExportMachines->setDisabled(true);

	QAction *examExport = new QAction(QString("Genera salvataggio"), this);
	connect(examExport, SIGNAL(triggered(bool)), this, SLOT(slotExamExport()));
	
	QMenu *examMenu = new QMenu(QString("Esame"));
	examMenu->addAction(examExport);
	ui->menubar->addMenu(examMenu);
#else
	connect(ui->actionVMLoad, SIGNAL(triggered(bool)), this, SLOT(slotVMLoad()));
	connect(ui->actionVMSave, SIGNAL(triggered(bool)), this, SLOT(slotVMSave()));
	connect(ui->actionVMSaveAs, SIGNAL(triggered(bool)), this, SLOT(slotVMSaveAs()));
#endif
	connect(ui->actionImportMachines, SIGNAL(triggered(bool)), this, SLOT(slotImportMachines()));
	connect(ui->actionExportMachines, SIGNAL(triggered(bool)), this, SLOT(slotExportMachines()));
	
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

	delete summaryDialog;
	delete vboxbridge;
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (!queryClose())
		event->ignore();
	else
		summaryDialog->close();
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

#ifdef EXAM_MODE
void MainWindow::setExamParams(QString _nome, QString _cognome, QString _matricola)
{
	launch_timestamp = new QDateTime();
	launch_timestamp->setTime(QTime::currentTime());
	launch_timestamp->setDate(QDate::currentDate());
	nome = _nome;
	cognome = _cognome;
	matricola = _matricola;
	std::cout << "launch_timestamp: " << launch_timestamp->toTime_t() << std::endl;
	std::cout << "Candidato: " << nome.toStdString() << " " << cognome.toStdString() << " (" << matricola.toStdString() << ")" << std::endl;
	
	QString newTitle = "";
	newTitle.append(windowTitle()).append(" - [").append(nome).append(" ").append(cognome).append(" (").append(matricola).append(")]");
	setWindowTitle(newTitle);
}

void MainWindow::slotExamExport()
{
	MachinesDialog machinesDialog(this, &VMTabSettings_vec);
	if(machinesDialog.buildDialog(true))
		machinesDialog.exec();
}
#else
bool MainWindow::slotVMSave()
{
	VMSettings *vmSettings = VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vmSettings;
	if (vmSettings->fileName.isEmpty())
		return slotVMSaveAs();
	
	bool write_done = vmSettings->save();
	if (!write_done)
	{
		QMessageBox::critical(this, "Errore di salvataggio",
				      "Impossibile scrivere sul file " + vmSettings->fileName);
		return false;
	}

	return write_done;
}

bool MainWindow::slotVMSaveAs()
{
	VMSettings *vmSettings = VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vmSettings;
	QString selectedFileName;
	if (vmSettings->fileName.isEmpty())
		selectedFileName = QFileDialog::getSaveFileName(this, "Salva macchina", VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->machine->getName(), "Machine VB-Ant file (*.vam)");
	else
		selectedFileName = QFileDialog::getSaveFileName(this, "Salva macchina", vmSettings->fileName, "Machine VB-Ant file (*.vam)");

	if (selectedFileName.isEmpty())
		return false;

	vmSettings->fileName = selectedFileName;
	return slotVMSave();
}

void MainWindow::slotVMLoad()
{
	VMSettings *vmSettings = VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vmSettings;
	const QString selectedFileName = QFileDialog::getOpenFileName(this, "Apri macchina", vmSettings->fileName, "Machine VB-Ant file (*.vam)");

	if (selectedFileName.isEmpty())
		return;

	settings_header_t settings_header;
	char *settings_ifaces;
	
	read_result_t read_result = vmSettings->read(&settings_header, &settings_ifaces, selectedFileName);

	switch(read_result)
	{
		case E_INVALID_FILE:
		case E_INVALID_HEADER:
		{
			QMessageBox qm(QMessageBox::Critical, "Ripristino impostazioni", QString::fromUtf8("Il file selezionato non è valido."), QMessageBox::Ok, this);
			qm.setPalette(palette());
			qm.exec();
			return;
		}
		case E_INVALID_CHECKSUM:
		{
			QMessageBox qm(QMessageBox::Warning, "Ripristino impostazioni", "Le impostazioni salvate potrebbero non essere valide.\nProvare a ripristinarle comunque?", QMessageBox::Yes | QMessageBox::No, this);
			qm.setPalette(palette());
			for(int i = 0; i < qm.buttons().size(); i++)
			{
				switch(qm.standardButton(qm.buttons()[i]))
				{
					case QDialogButtonBox::Yes: qm.buttons()[i]->setText(QString::fromUtf8("Sì")); break;
					case QDialogButtonBox::No: qm.buttons()[i]->setText("No"); break;
				}
			}

			if(qm.exec() == QMessageBox::Yes)
				break;
			return;
		}
		case E_MACHINE_MISMATCH:
		{
			QMessageBox qm(QMessageBox::Warning, "Ripristino impostazioni", "Le impostazioni salvate non provengono dalla macchina selezionata.\nRipristinarle comunque?", QMessageBox::Yes | QMessageBox::No, this);
			qm.setPalette(palette());
			for(int i = 0; i < qm.buttons().size(); i++)
			{
				switch(qm.standardButton(qm.buttons()[i]))
				{
					case QDialogButtonBox::Yes: qm.buttons()[i]->setText(QString::fromUtf8("Sì")); break;
					case QDialogButtonBox::No: qm.buttons()[i]->setText("No"); break;
				}
			}

			if(qm.exec() == QMessageBox::Yes)
				break;
			return;
		}
		case NO_ERROR:
			break;
		default:
			return;
	}

	vmSettings->load(settings_header, settings_ifaces);
	vmSettings->restore();
}

#endif
void MainWindow::slotExportMachines()
{
	MachinesDialog machinesDialog(this, &VMTabSettings_vec);
	if(machinesDialog.buildDialog())
		machinesDialog.exec();
}

void MainWindow::slotImportMachines()
{
	const QString fileName = QFileDialog::getOpenFileName(this, "Importa macchine", "", "Machine set VB-Ant file (*.vas)");
	if(fileName == "")
		return;

	MachinesDialog machinesDialog(this, &VMTabSettings_vec, fileName);
	if(machinesDialog.buildDialog())
		machinesDialog.exec();
}

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

void MainWindow::slotEnableAll()
{
	for(int i = 0; i < ui->vm_tabs->count(); i++)
	{
		uint32_t machineState = VMTabSettings_vec.at(i)->machine->getState();

		if(machineState != MachineState::Running &&
			machineState != MachineState::Paused &&
			machineState != MachineState::Starting)
			VMTabSettings_vec.at(i)->vm_enabled->setChecked(true);
	}
}

void MainWindow::slotDisableAll()
{
	for(int i = 0; i < ui->vm_tabs->count(); i++)
	{
		uint32_t machineState = VMTabSettings_vec.at(i)->machine->getState();
		
		if(machineState != MachineState::Running &&
			machineState != MachineState::Paused &&
			machineState != MachineState::Starting)
			VMTabSettings_vec.at(i)->vm_enabled->setChecked(false);
	}
}

void MainWindow::slotShowSummary()
{
	summaryDialog->refresh();
	summaryDialog->show();
	summaryDialog->raise();
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

void MainWindow::slotNew()
{
	CloneDialog c(this, true);
	c.exec();
	emit machinesPoolChanged();
}

void MainWindow::slotClone()
{
	CloneDialog c(this);
	c.exec();
	emit machinesPoolChanged();
}

int MainWindow::launchCreateProcess(QString qName, bool reInitIfaces, bool restoreFromFile)
{
	ProgressDialog p("");
	p.ui->label->setText(QString::fromUtf8("Caricamento macchina \"").append(qName).append("\""));
	p.ui->progressBar->setValue(0);
	p.open();

	VMTabSettings *vmTabSettings = addMachine(vboxbridge->newVM(qName));
	if(vmTabSettings == NULL)
		return -1;

	if(!restoreFromFile)
	{
		vmTabSettings->vm->cleanIfaces(VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->ifaces, VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->ifaces_size);
		vmTabSettings->vm->saveSettings();
		vmTabSettings->refreshTable();
	}

	int newTab = ui->vm_tabs->addTab(vmTabSettings, qName);
	VMTabSettings_vec.push_back(vmTabSettings);
	p.ui->progressBar->setValue(100);

	ui->vm_tabs->setCurrentIndex(newTab);

	return newTab;
}

void MainWindow::launchCloneProcess(QString qName, bool reInitIfaces)
{
	ProgressDialog p("");
	p.ui->label->setText(QString::fromUtf8("Caricamento macchina \"").append(qName).append("\""));
	p.ui->progressBar->setValue(0);
	p.open();

	VMTabSettings *vmTabSettings = addMachine(VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->clone(qName, reInitIfaces));
	if(vmTabSettings == NULL)
		return;

	vmTabSettings->vm->cleanIfaces(VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->ifaces, VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->ifaces_size);
	vmTabSettings->vm->copyIfaces(VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->ifaces, VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->ifaces_size);
	vmTabSettings->vm->saveSettings();
	vmTabSettings->refreshTable();

	int newTab = ui->vm_tabs->addTab(vmTabSettings, qName);
	VMTabSettings_vec.push_back(vmTabSettings);
	p.ui->progressBar->setValue(100);

	ui->vm_tabs->setCurrentIndex(newTab);
}

VMTabSettings *MainWindow::addMachine(IMachine *m)
{
	if(m == NULL)
	{
		QMessageBox qm(QMessageBox::Critical, "Errore", "Errore durante la creazione della macchina virtuale", QMessageBox::Close, this);
		qm.setPalette(palette());
		for(int i = 0; i < qm.buttons().size(); i++)
		{
			switch(qm.standardButton(qm.buttons()[i]))
			{
				case QDialogButtonBox::Close: qm.buttons()[i]->setText("Chiudi"); break;
			}
		}
		qm.exec();
		return NULL;
	}

	OSBridge::unloadNbdModule();
	OSBridge::loadNbdModule(machines_vec.size() + 1);

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

	return vmSettings;
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
			{
				if(i != tabIndex)
				{
					VMTabSettings_vec_shadow.push_back(VMTabSettings_vec.at(i));
					machines_vec_shadow.push_back(machines_vec.at(i));
				}
			}

			ui->vm_tabs->removeTab(tabIndex);

			VMTabSettings *v = VMTabSettings_vec.at(tabIndex);
			MachineBridge *mb = machines_vec.at(tabIndex);
			VMTabSettings_vec = VMTabSettings_vec_shadow;
			machines_vec = machines_vec_shadow;

			delete v;
			delete mb;
		}
		emit machinesPoolChanged();
	}
}

void MainWindow::slotRename()
{
	VMTabSettings *vmtab = VMTabSettings_vec.at(ui->vm_tabs->currentIndex());

	QDialog d(this);
	QVBoxLayout verticalLayout(&d);
	QLabel label(QString("Rinomina macchina \"").append(vmtab->getMachineName()).append("\""), &d);
	QLineEdit lineEdit(vmtab->getMachineName(), &d);
	QDialogButtonBox buttonBox(&d);
	buttonBox.setStandardButtons(QDialogButtonBox::StandardButton::Ok | QDialogButtonBox::StandardButton::Cancel);
	d.connect(&buttonBox, SIGNAL(accepted()), &d, SLOT(accept()));
	d.connect(&buttonBox, SIGNAL(rejected()), &d, SLOT(reject()));

	verticalLayout.addWidget(&label);
	verticalLayout.addWidget(&lineEdit);
	verticalLayout.addWidget(&buttonBox);

	d.setLayout(&verticalLayout);
	d.setPalette(palette());

	for(;;)
	{
		if(d.exec() != QDialog::Accepted)
			break;

		if(lineEdit.text().length() > 0 && lineEdit.text() == vboxbridge->validateMachineName(lineEdit.text(), VMTabSettings_vec.size()))
		{
			if(vmtab->setMachineName(lineEdit.text()))
			{
				std::cout << "new_name: " << lineEdit.text().toStdString() << std::endl;
				ui->vm_tabs->setTabText(ui->vm_tabs->currentIndex(), lineEdit.text());
			}
			emit machinesPoolChanged();
			break;
		}
		else
		{
			QMessageBox qm(QMessageBox::Warning, "Errore", "Il nome specificato non è valido", QMessageBox::Close, this);
			qm.setPalette(palette());
			qm.exec();
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

	VMTabSettings_vec.at(tabIndex)->vm->refreshIface(machine->getIfaceSlot(nic), nic);
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
			ui->actionRinomina->setEnabled(false);
			ui->actionClona->setEnabled(false);
			ui->actionElimina->setEnabled(false);
			ui->actionAvviaAll->setEnabled(true);
			ui->actionInterrompiAll->setEnabled(true);
			ui->actionAvvia->setEnabled(false);
			ui->actionPausa->setEnabled(true); ui->actionPausa->setChecked(false);
			ui->actionReset->setEnabled(true);
			ui->actionInterrompi->setEnabled(true);
			ui->actionVMLoad->setEnabled(false);

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
			ui->actionRinomina->setEnabled(false);
			ui->actionClona->setEnabled(false);
			ui->actionElimina->setEnabled(false);
			ui->actionAvviaAll->setEnabled(true);
			ui->actionInterrompiAll->setEnabled(true);
			ui->actionAvvia->setEnabled(false);
			ui->actionPausa->setEnabled(true); ui->actionPausa->setChecked(true);
			ui->actionReset->setEnabled(false);
			ui->actionInterrompi->setEnabled(true);
			ui->actionVMLoad->setEnabled(false);
			
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
			ui->actionRinomina->setEnabled(true);
			ui->actionClona->setEnabled(true);
			ui->actionElimina->setEnabled(true);
			ui->actionAvviaAll->setEnabled(true);
			ui->actionInterrompiAll->setEnabled(true);
			ui->actionAvvia->setEnabled(true);
			ui->actionPausa->setEnabled(false); ui->actionPausa->setChecked(false);
			ui->actionReset->setEnabled(false);
			ui->actionInterrompi->setEnabled(false);
#ifndef EXAM_MODE
			ui->actionVMLoad->setEnabled(true);
#endif

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
