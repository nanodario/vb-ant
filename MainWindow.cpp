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
: QMainWindow(parent), ui(new Ui_MainWindow), vboxbridge(new VirtualBoxBridge()), machines_vec(vboxbridge->getMachines())
{
	VMTabSettings_vec.clear();
	ui->setupUi(this);
	
	int i;
	for (i = 0; i < machines_vec.size(); i++)
	{
		QString tabname = MachineBridge::getName(machines_vec.at(i));
		VMTabSettings *vmSettings = new VMTabSettings(ui->vm_tabs, tabname, vboxbridge, machines_vec.at(i));
		ui->vm_tabs->addTab(vmSettings, tabname);
// 		vmSettings->addTo(ui->vm_tabs);
		VMTabSettings_vec.push_back(vmSettings);
	}

	connect(ui->actionInfo_su, SIGNAL(triggered(bool)), this, SLOT(slotInfo()));
	connect(ui->actionNew, SIGNAL(triggered(bool)), this, SLOT(slotActionNew()));
	connect(ui->actionopen, SIGNAL(triggered(bool)), this, SLOT(slotActionOpen()));
	connect(ui->actionSave, SIGNAL(triggered(bool)), this, SLOT(slotActionSave()));
	connect(ui->actionSaveAs, SIGNAL(triggered(bool)), this, SLOT(slotActionSaveAs()));
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
		machines_vec.back() = nsnull;
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
