#include "MainWindow.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <sstream>
#include <iostream>
#include <bitset>
#include "stdint.h"

#include "ui_MainWindow.h"
#include "VMTabSettings.h"
#include "InfoDialog.h"

MainWindow::MainWindow(const QString &fileToOpen, QWidget *parent)
: QMainWindow(parent), ui(new Ui_MainWindow)
{
	VMTabSettings_vec.clear();
	ui->setupUi(this);
	
	int i;
	for (i = 0; i < 4; i++)
	{
		char tabname[30];
		sprintf(tabname, "Macchina %d", i+1);
		VMTabSettings *vmSettings = new VMTabSettings(tabname);
		vmSettings->addTo(ui->vm_tabs);
		VMTabSettings_vec.push_back(vmSettings);
	}
// 	VMTabSettings_vec.at(0)->ifaces_table->addIface("prova1", "00445667788");
// 	VMTabSettings_vec.at(0)->ifaces_table->setIp(0, "192.168.0.2");
// 	
// 	VMTabSettings_vec.at(0)->ifaces_table->addIface("prova2", "00hh22334478");
// 	VMTabSettings_vec.at(0)->ifaces_table->setIp(1, "10...0");
// 	
// 	VMTabSettings_vec.at(0)->ifaces_table->addIface("prova3", "33-11-55-77-66-5");
// 	VMTabSettings_vec.at(0)->ifaces_table->setIp(2, "310.3.1.2");
// 	
// 	VMTabSettings_vec.at(0)->ifaces_table->addIface("prova4", "33-11-55-77-66-553");
// 	VMTabSettings_vec.at(0)->ifaces_table->setIp(3, "0.0.3.1");
// 	
// 	VMTabSettings_vec.at(0)->ifaces_table->addIface("prova5", "3311.5577.6655");
// 	VMTabSettings_vec.at(0)->ifaces_table->setIp(4, "255.255.255.255");
// 	
// 	VMTabSettings_vec.at(0)->ifaces_table->addIface("prova6", "h311.5577.6655");
// 	VMTabSettings_vec.at(0)->ifaces_table->setIp(5, "255.4.5.1");
// 	
// 	VMTabSettings_vec.at(0)->ifaces_table->addIface("prova7", "k3-11-55-77-66-55");
// 	VMTabSettings_vec.at(0)->ifaces_table->addIface("prova8", "h3:kk:55:77:66:55");
	
	connect(ui->actionInfo_su, SIGNAL(activated()), this, SLOT(slotInfo()));
// 	connect(ui->actionOpen, SIGNAL(activated()), this, SLOT(slotOpen()));
// 	connect(ui->actionSave, SIGNAL(activated()), this, SLOT(slotSave()));
// 	connect(ui->actionSaveAs, SIGNAL(activated()), this, SLOT(slotSaveAs()));

}

MainWindow::~MainWindow()
{
	while(!VMTabSettings_vec.empty())
	{
		delete VMTabSettings_vec.back();
		VMTabSettings_vec.pop_back();
	}

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

void MainWindow::slotOpen()
{
	/*
	const QString selectedFileName = QFileDialog::getOpenFileName(this,
		"Apri documento", fileName, "Grafo (*.graph)");

	if (selectedFileName.isEmpty() || !queryClose())
		return;

	loadFile(selectedFileName);
	*/
}

bool MainWindow::slotSave()
{
	/*
	if (fileName.isEmpty())
		return slotSaveAs();

	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
	{
		QMessageBox::critical(this, "Errore di salvataggio",
			"Impossibile scrivere sul file " + fileName);
		return false;
	}

	QTextStream(&file) << ui->campo->toXmlString();
	ui->campo->setCleanUndoHistory();
	file.close();

	return true;
	*/
	return true;
}

bool MainWindow::slotSaveAs()
{
	/*
	const QString selectedFileName = QFileDialog::getSaveFileName(this,
		"Salva documento", fileName, "Grafo (*.graph)");

	if (selectedFileName.isEmpty())
		return false;

	fileName = selectedFileName;
	return slotSave();
	*/
	return true;
}

void MainWindow::slotInfo()
{
	infoDialog.show();
}
