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
	connect(ui->actionImpostazioni, SIGNAL(triggered(bool)), this, SLOT(slotSettings()));
	
	connect(ui->toolbarAvvia, SIGNAL(triggered(bool)), ui->actionAvvia, SIGNAL(triggered(bool)));
	connect(ui->toolbarImpostazioni, SIGNAL(triggered(bool)), ui->actionImpostazioni, SIGNAL(triggered(bool)));
}

MainWindow::~MainWindow()
{
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
	std::cout << "[" << __func__ << "] tab:" << tab << std::endl;
// 	std::cout << "Selezionata tab \"" << ui->vm_tabs->widget(tab)->objectName().toStdString() << "\"" << std::endl;
}

void MainWindow::slotStart()
{
	VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->start();
}

void MainWindow::slotPause()
{
	VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->pause();
}

void MainWindow::slotReset()
{
	VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->reset();
}

void MainWindow::slotStop()
{
	VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->stop();
}

void MainWindow::slotSettings()
{
	VMTabSettings_vec.at(ui->vm_tabs->currentIndex())->vm->openSettings();
}
