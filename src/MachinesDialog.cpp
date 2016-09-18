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

#include "MachinesDialog.h"
#include "MainWindow.h"
#include "VMTabSettings.h"
#include <stdlib.h>
#include <vector>
#include <sstream>
#include <QString>
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QCloseEvent>

#ifdef USE_ZLIB
	#include "ZlibWrapper.h"
#endif

#ifdef EXAM_MODE
	#include "GPGMEWrapper.h"
#endif

#include "ui_MachinesDialog.h"

#ifdef CONFIGURABLE_IP
#define HORIZONTAL_HEADERS "Interfaccia;Indirizzo MAC;Indirizzo IP;Subnet mask"
#else
#define HORIZONTAL_HEADERS "Interfaccia;Indirizzo MAC"
#endif

MachinesDialog::MachinesDialog(MainWindow *mainwindow, std::vector<VMTabSettings*> *vmTab_vec, QString fileName): QDialog()
, mainwindow(mainwindow), ui(new Ui_MachinesDialog), vmTab_vec(vmTab_vec), fileName(fileName), settings_header(NULL), settings_ifaces(NULL), machines_number(0)
{
// 	buildDialog();
}

MachinesDialog::MachinesDialog(MainWindow *mainwindow, std::vector<VMTabSettings*> *vmTab_vec, QPalette palette, QString fileName): QDialog()
, mainwindow(mainwindow), ui(new Ui_MachinesDialog), vmTab_vec(vmTab_vec), fileName(fileName), settings_header(NULL), settings_ifaces(NULL), machines_number(0)
{
// 	buildDialog();
	setPalette(palette);
}

MachinesDialog::~MachinesDialog()
{
	while(ui->treeWidget->topLevelItemCount() > 0)
	{
		QTreeWidgetItem *item = ui->treeWidget->topLevelItem(0);
		while(item->childCount() > 0)
		{
			QTreeWidgetItem *child = item->child(0);
			item->removeChild(child);
			delete child;
		}
		delete item;
	}

	if(settings_ifaces != NULL)
	{
		for(int i = 0; i < machines_number; i++)
			free(settings_ifaces[i]);
		free(settings_ifaces);
	}

	if(settings_header != NULL)
		free(settings_header);

	delete ui;
}

#ifndef EXAM_MODE
bool MachinesDialog::buildDialog()
#else
bool MachinesDialog::buildDialog(bool examMode)
#endif
{
	ui->setupUi(this);
	ui->retranslateUi(this);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
	buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
	buttonBox->setStandardButtons(QDialogButtonBox::NoButton);
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));

	QStringList horizontalHeaderLabels = QString(HORIZONTAL_HEADERS).split(";");
	ui->treeWidget->setColumnCount(horizontalHeaderLabels.count());
	ui->treeWidget->setHeaderLabels(horizontalHeaderLabels);
	ui->treeWidget->setSelectionMode(QAbstractItemView::NoSelection);

	if(fileName == "")
	{
		QHBoxLayout *horizontalLayout = new QHBoxLayout(this);
		horizontalLayout->setObjectName("horizontalLayout");

		checkBox = new QCheckBox(this);
		checkBox->setObjectName("checkBox");
		checkBox->setText("Comprimi file");

		horizontalLayout->addWidget(checkBox);
		horizontalLayout->addWidget(buttonBox);
		ui->verticalLayout->addLayout(horizontalLayout);

		setWindowTitle(QApplication::translate("SummaryDialog", "Esporta macchine", 0, QApplication::UnicodeUTF8));
		for(int row = 0; row < vmTab_vec->size(); row++)
		{
			VMTabSettings *vmTabSettings = vmTab_vec->at(row);
			
			QTreeWidgetItem *item = new QTreeWidgetItem();
			item->setCheckState(0, Qt::Unchecked);
			item->setText(0, QString("Macchina: ").append(vmTabSettings->getMachineName()));
			
			for(int iface_index = 0; iface_index < vmTabSettings->vm->ifaces_size; iface_index++)
			{
				QTreeWidgetItem *childItem = new QTreeWidgetItem(item);
				childItem->setText(0, vmTabSettings->vm->ifaces[iface_index]->name);
				childItem->setText(1, vmTabSettings->vm->ifaces[iface_index]->mac);
#ifdef CONFIGURABLE_IP
				childItem->setText(2, vmTabSettings->vm->ifaces[iface_index]->ip);
				childItem->setText(3, vmTabSettings->vm->ifaces[iface_index]->subnetMask);
#endif
			}

			ui->treeWidget->addTopLevelItem(item);
			ui->treeWidget->header()->setResizeMode(QHeaderView::ResizeToContents);
		}

		buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Save);
#ifndef USE_ZLIB
		checkBox->setChecked(false);
		checkBox->setEnabled(false);
#endif		
#ifdef EXAM_MODE
		if(examMode)
			connect(buttonBox, SIGNAL(accepted()), this, SLOT(slotExamExportMachines()));
		else
#endif
		{
			connect(buttonBox, SIGNAL(accepted()), this, SLOT(slotExportMachines()));
		}
	}
	else
	{
		ui->verticalLayout->addWidget(buttonBox);
		setWindowTitle(QApplication::translate("SummaryDialog", "Importa macchine", 0, QApplication::UnicodeUTF8));

		read_result_t read_result = loadMachines(&settings_header, &settings_ifaces, &machines_number);

		switch(read_result)
		{
			case NO_ERROR:
				break;
			case E_UNINMPLEMENTED:
			{
				QMessageBox qm(QMessageBox::Critical, "Ripristino impostazioni", QString::fromUtf8("Funzione non implementata."), QMessageBox::Ok, this);
				qm.setPalette(palette());
				qm.exec();
				return false;
			}				
			case E_INVALID_FILE:
			case E_INVALID_HEADER:
			default:
			{
				QMessageBox qm(QMessageBox::Critical, "Ripristino impostazioni", QString::fromUtf8("Il file selezionato non è valido."), QMessageBox::Ok, this);
				qm.setPalette(palette());
				qm.exec();
				return false;
			}
		}

		for(int i = 0; i < machines_number; i++)
		{
			QTreeWidgetItem *item = new QTreeWidgetItem();
			item->setCheckState(0, Qt::Unchecked);
			item->setText(0, QString("Macchina: ").append(settings_header[i].machine_name));

			for(int iface_index = 0; iface_index < settings_header[i].settings_iface_size; iface_index++)
			{
				QTreeWidgetItem *childItem = new QTreeWidgetItem(item);
				childItem->setText(0, settings_ifaces[i][iface_index].name);
				childItem->setText(1, settings_ifaces[i][iface_index].mac);
#ifdef CONFIGURABLE_IP
				childItem->setText(2, settings_ifaces[i][iface_index].ip);
				childItem->setText(3, settings_ifaces[i][iface_index].subnetMask);
#endif
			}

			ui->treeWidget->addTopLevelItem(item);
			ui->treeWidget->header()->setResizeMode(QHeaderView::ResizeToContents);
		}

		buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Open);
		connect(buttonBox, SIGNAL(accepted()), this, SLOT(slotImportMachines()));
	}
	return true;
}

void MachinesDialog::slotExportMachines()
{
	for(int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
	{
		if(ui->treeWidget->topLevelItem(i)->checkState(0) == Qt::Checked)
		{
			fileName = QFileDialog::getSaveFileName(this, "Salva macchine", "", "Machine set VB-Ant file (*.vas)");
			if(fileName == "")
				return;
			std::vector<VirtualMachine*> vm_vec;

			for(int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
				if(ui->treeWidget->topLevelItem(i)->checkState(0) == Qt::Checked)
					vm_vec.push_back(vmTab_vec->at(i)->vm);

			saveMachines(vm_vec, checkBox->isChecked());
			close();
			return;
		}
	}

	QMessageBox qm(QMessageBox::Information, "Esportazione macchine", "Selezionare almeno una macchina da esportare", QMessageBox::Ok, this);
	qm.setPalette(palette());
	qm.exec();
}

#ifdef EXAM_MODE
void MachinesDialog::slotExamExportMachines()
{
	for(int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
	{
		if(ui->treeWidget->topLevelItem(i)->checkState(0) == Qt::Checked)
		{
			fileName = QFileDialog::getSaveFileName(this, "Salva macchine (esame)", "", "Exam machine set VB-Ant file (*.vax)");
			if(fileName == "")
				return;
			std::vector<VirtualMachine*> vm_vec;
			
			for(int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
				if(ui->treeWidget->topLevelItem(i)->checkState(0) == Qt::Checked)
					vm_vec.push_back(vmTab_vec->at(i)->vm);
				
				saveMachines(vm_vec, checkBox->isChecked(), true);
			close();
			return;
		}
	}
	
	QMessageBox qm(QMessageBox::Information, "Esportazione macchine", "Selezionare almeno una macchina da esportare", QMessageBox::Ok, this);
	qm.setPalette(palette());
	qm.exec();
}
#endif

void MachinesDialog::slotImportMachines()
{
	std::vector<int> vm_selected;
	std::vector<int> vm_update;
	std::vector<int> vm_update_data;
	std::vector<int> vm_executing;
	std::vector<int> vm_executing_data;
	std::vector<int> vm_create;

	for(int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
		if(ui->treeWidget->topLevelItem(i)->checkState(0) == Qt::Checked)
			vm_selected.push_back(i);

	for(int i = 0; i < vm_selected.size(); i++)
	{
		bool existing_machine = false;
		for(int j = 0; j < vmTab_vec->size(); j++)
		{
			if(vmTab_vec->at(j)->getMachineUUID() == settings_header[vm_selected.at(i)].machine_uuid)
			{
				uint32_t machineState = vmTab_vec->at(j)->machine->getState();

				if(machineState == MachineState::Running ||
					machineState == MachineState::Paused ||
					machineState == MachineState::Starting)
				{
					vm_executing.push_back(j);
					vm_executing_data.push_back(i);
				}
				else
				{
					vm_update.push_back(j);
					vm_update_data.push_back(i);
				}

				existing_machine = true;
				break;
			}
		}
		if(!existing_machine)
			vm_create.push_back(vm_selected.at(i));
	}

	if(vm_create.size() > 0)
	{
		QDialog dialog(this);
		dialog.resize(500, 300);
		QVBoxLayout *verticalLayout = new QVBoxLayout(&dialog);
		QLabel label("Alcune macchine selezionate non esistono. Creare le seguenti macchine?", &dialog);
		QTreeWidget treeWidget(&dialog);
		QDialogButtonBox buttonBox(&dialog);
		buttonBox.setStandardButtons(QDialogButtonBox::Yes|QDialogButtonBox::No);

		verticalLayout->addWidget(&label);
		verticalLayout->addWidget(&treeWidget);
		verticalLayout->addWidget(&buttonBox);

		QStringList horizontalHeaderLabels = QString(HORIZONTAL_HEADERS).split(";");
		treeWidget.setColumnCount(horizontalHeaderLabels.count());
		treeWidget.setHeaderLabels(horizontalHeaderLabels);
		treeWidget.setSelectionMode(QAbstractItemView::NoSelection);

		for(int i = 0; i < vm_create.size(); i++)
		{
			QTreeWidgetItem *item = new QTreeWidgetItem();
// 			item->setCheckState(0, Qt::Unchecked);
			item->setText(0, QString("Macchina: ").append(settings_header[vm_create.at(i)].machine_name));
			
			for(int iface_index = 0; iface_index < settings_header[vm_create.at(i)].settings_iface_size; iface_index++)
			{
				QTreeWidgetItem *childItem = new QTreeWidgetItem(item);
				childItem->setText(0, settings_ifaces[vm_create.at(i)][iface_index].name);
				childItem->setText(1, settings_ifaces[vm_create.at(i)][iface_index].mac);
#ifdef CONFIGURABLE_IP
				childItem->setText(2, settings_ifaces[vm_create.at(i)][iface_index].ip);
				childItem->setText(3, settings_ifaces[vm_create.at(i)][iface_index].subnetMask);
#endif
			}

			treeWidget.addTopLevelItem(item);
			treeWidget.header()->setResizeMode(QHeaderView::ResizeToContents);
		}

		connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
		connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
		dialog.setLayout(verticalLayout);

		if(dialog.exec() == QDialog::Rejected)
			vm_create.clear();
	}

	if(vm_executing.size() > 0)
	{
		QDialog dialog(this);
		dialog.resize(500, 300);
		QVBoxLayout *verticalLayout = new QVBoxLayout(&dialog);
		QLabel label(QString::fromUtf8("Non è possibile importare le impostazioni per le macchine in esecuzione.\nArrestare le macchine selezionate?"), &dialog);
		QTreeWidget treeWidget(&dialog);
		QDialogButtonBox buttonBox(&dialog);
		buttonBox.setStandardButtons(QDialogButtonBox::Yes|QDialogButtonBox::No);

		verticalLayout->addWidget(&label);
		verticalLayout->addWidget(&treeWidget);
		verticalLayout->addWidget(&buttonBox);

		QStringList horizontalHeaderLabels = QString(HORIZONTAL_HEADERS).split(";");
		treeWidget.setColumnCount(horizontalHeaderLabels.count());
		treeWidget.setHeaderLabels(horizontalHeaderLabels);
		treeWidget.setSelectionMode(QAbstractItemView::NoSelection);

		for(int i = 0; i < vm_executing.size(); i++)
		{
			uint32_t machineState = vmTab_vec->at(vm_executing.at(i))->machine->getState();

			QTreeWidgetItem *item = new QTreeWidgetItem();
			item->setCheckState(0, Qt::Checked);
			item->setText(0, QString("Macchina: ").append(vmTab_vec->at(vm_executing.at(i))->getMachineName()));

			for(int iface_index = 0; iface_index < settings_header[vm_executing_data.at(i)].settings_iface_size; iface_index++)
			{
				QTreeWidgetItem *childItem = new QTreeWidgetItem(item);
				childItem->setText(0, settings_ifaces[vm_executing_data.at(i)][iface_index].name);
				childItem->setText(1, settings_ifaces[vm_executing_data.at(i)][iface_index].mac);
#ifdef CONFIGURABLE_IP
				childItem->setText(2, settings_ifaces[vm_executing_data.at(i)][iface_index].ip);
				childItem->setText(3, settings_ifaces[vm_executing_data.at(i)][iface_index].subnetMask);
#endif
			}

			treeWidget.addTopLevelItem(item);
			treeWidget.header()->setResizeMode(QHeaderView::ResizeToContents);
		}

		connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
		connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
		dialog.setLayout(verticalLayout);

		if(dialog.exec() != QDialog::Accepted)
			vm_executing.clear();
		else
		{
			std::vector<int> vm_executing_temp;
			std::vector<int> vm_executing_data_temp;
			for(int i = 0; i < treeWidget.topLevelItemCount(); i++)
				if(treeWidget.topLevelItem(i)->checkState(0) == Qt::Checked)
				{
					vm_executing_temp.push_back(vm_executing.at(i));
					vm_executing_data_temp.push_back(vm_executing_data.at(i));
				}

			vm_executing = vm_executing_temp;
			vm_executing_data = vm_executing_data_temp;
		}
	}

	for(int i = 0; i < vm_executing.size(); i++)
	{
		uint32_t machineState = vmTab_vec->at(vm_executing.at(i))->machine->getState();

		if(machineState == MachineState::Running ||
		   machineState == MachineState::Paused ||
		   machineState == MachineState::Starting)
			vmTab_vec->at(vm_executing.at(i))->machine->stop(true);

		if(!updateMachine(vmTab_vec->at(vm_executing.at(i)), settings_header[vm_executing_data.at(i)], settings_ifaces[vm_executing_data.at(i)]))
		{
			QMessageBox qm(QMessageBox::Critical, "Errore",
				       QString("Errore durante l'aggiornamento della macchina ").append(settings_header[vm_executing_data.at(i)].machine_name),
				       QMessageBox::StandardButton::Ok);
			qm.setPalette(palette());
			qm.exec();
		}
	}

	for(int i = 0; i < vm_update.size(); i++)
		if(!updateMachine(vmTab_vec->at(vm_update.at(i)), settings_header[vm_update_data.at(i)], settings_ifaces[vm_update_data.at(i)]))
		{
			QMessageBox qm(QMessageBox::Critical, "Errore",
				       QString("Errore durante l'aggiornamento della macchina ").append(settings_header[vm_update_data.at(i)].machine_name),
				       QMessageBox::StandardButton::Ok);
			qm.setPalette(palette());
			qm.exec();
		}

	for(int i = 0; i < vm_create.size(); i++)
		if(!createMachine(settings_header[vm_create.at(i)], settings_ifaces[vm_create.at(i)]))
		{
			QMessageBox qm(QMessageBox::Critical, "Errore",
				       QString("Errore durante la creazione della macchina ").append(settings_header[vm_create.at(i)].machine_name),
				       QMessageBox::StandardButton::Ok);
			qm.setPalette(palette());
			qm.exec();
		}

	close();
}

#ifndef EXAM_MODE
bool MachinesDialog::saveMachines(std::vector<VirtualMachine*> vm_vec, bool deflate)
#else
bool MachinesDialog::saveMachines(std::vector<VirtualMachine*> vm_vec, bool deflate, bool examMode)
#endif
{
	uint32_t total_size = 0;
	char **serialized_machines = (char **)malloc(vm_vec.size() * sizeof(char*));
	uint32_t *serialized_machines_sizes = (uint32_t *)malloc(vm_vec.size() * sizeof(uint32_t));

	for(int i = 0; i < vm_vec.size(); i++)
	{
		settings_header_t settings_header;
		char *serialized_ifaces = NULL;
		uint32_t serialized_ifaces_size = vm_vec.at(i)->vmSettings->get_serializable_machine(&settings_header, &serialized_ifaces);
		total_size += sizeof(uint32_t) + (serialized_ifaces_size + sizeof(settings_header_t)) * sizeof(char);

		serialized_machines_sizes[i] = serialized_ifaces_size + sizeof(settings_header_t);
		serialized_machines[i] = (char *)malloc(sizeof(uint32_t) + (serialized_ifaces_size + sizeof(settings_header_t)) * sizeof(char));

		memcpy(serialized_machines[i], &(serialized_machines_sizes[i]), sizeof(uint32_t));
		memcpy((serialized_machines[i])+sizeof(uint32_t), &settings_header, sizeof(settings_header_t));
		memcpy((serialized_machines[i])+(sizeof(uint32_t)+sizeof(settings_header_t)), serialized_ifaces, serialized_ifaces_size);
	}

	char *serialized_data = (char *)malloc(total_size);
	memset(serialized_data, 0, total_size);
	int copied_bytes = 0;

	for(int i = 0; i < vm_vec.size(); i++)
	{
		memcpy(serialized_data+copied_bytes, serialized_machines[i], serialized_machines_sizes[i] * sizeof(char) + sizeof(uint32_t));
		copied_bytes += serialized_machines_sizes[i] + sizeof(uint32_t);
		free(serialized_machines[i]);
	}
	free(serialized_machines);
	free(serialized_machines_sizes);

	bool retval;
	QFile file(fileName);
	if(file.open(QIODevice::WriteOnly))
	{
		uint8_t magicBytes_size = sizeof(uint8_t) + sizeof(uint32_t) + sizeof(char) * (strlen(SAVEFILE_MAGIC_BYTES)+2);
		char *magicBytes = (char *)malloc(magicBytes_size + 1);
		memset(magicBytes, 0, magicBytes_size + 1);
		uint8_t vm_size = vm_vec.size();

		magicBytes[0] = 'S';
		magicBytes[2] = vm_size;
		strncpy(magicBytes + 2 + sizeof(uint8_t) + sizeof(uint32_t), SAVEFILE_MAGIC_BYTES, strlen(SAVEFILE_MAGIC_BYTES)+1);

		uint32_t bytes_written = 0;
		uint32_t expected_size = 0;

		if(0);
#ifdef EXAM_MODE
		else if(examMode)
		{ //Exam mode format
			magicBytes[1] = 'X';
			memcpy(magicBytes + 2 + sizeof(uint8_t), &total_size, sizeof(uint32_t));
			bytes_written = file.write(magicBytes, magicBytes_size);
// 			bytes_written += file.write(DATA, SIZE); //TODO

			file.flush();
			expected_size = total_size;
			free(magicBytes);
		}
#endif
#ifdef USE_ZLIB
		else if(deflate)
		{ // Zlib format
			char *z_serialized_data;
			int z_serialized_data_size = ZlibWrapper::def(&z_serialized_data, serialized_data, total_size, -1);

			magicBytes[1] = 'Z';
			memcpy(magicBytes + 2 + sizeof(uint8_t), &z_serialized_data_size, sizeof(uint32_t));
			bytes_written = file.write(magicBytes, magicBytes_size);
			bytes_written += file.write(z_serialized_data, z_serialized_data_size);

			file.flush();
			expected_size = z_serialized_data_size;

			free(magicBytes);
			free(z_serialized_data);
		}
#endif
		else
		{ // Plain text format
			magicBytes[1] = 'P';
			memcpy(magicBytes + 2 + sizeof(uint8_t), &total_size, sizeof(uint32_t));
			bytes_written = file.write(magicBytes, magicBytes_size);
			bytes_written += file.write(serialized_data, total_size);

			file.flush();
			expected_size = total_size;
			free(magicBytes);
		}

		file.close();

		std::cout << "Expected size: " << (expected_size + magicBytes_size) << std::endl;;
		std::cout << "Written size:  " << bytes_written << std::endl;
		retval = (bytes_written == expected_size + magicBytes_size);
	}
	else
		retval = false;

	free(serialized_data);
	return retval;
}

read_result_t MachinesDialog::loadMachines(settings_header_t **settings_header, settings_iface_t ***settings_ifaces, uint32_t *machines_number)
{
	QFile file(fileName);
	if(!file.open(QIODevice::ReadOnly))
		return E_UNKNOWN;

#ifdef USE_ZLIB
	bool use_zlib = false;
#endif
#ifdef EXAM_MODE
	bool exam_mode = false;
#endif
	
	//read header
	char *magicbytes = (char *)malloc(3*sizeof(char)+sizeof(uint32_t));
	int magicbytes_read = file.read(magicbytes, 3*sizeof(char)+sizeof(uint32_t));
	QByteArray qMagicbytes = file.readLine();

	if(magicbytes_read < 0 || magicbytes[0] != 'S' || strncmp(qMagicbytes.data(), SAVEFILE_MAGIC_BYTES, strlen(PROGRAM_NAME)))
		return E_INVALID_FILE;
	else
	{
		std::cout << "magicbytes[1]: " << magicbytes[1] << std::endl;
		switch(magicbytes[1])
		{
			case 'P':
				break;
#ifdef USE_ZLIB
			case 'Z':
				use_zlib = true;
				break;
#endif
#ifdef EXAM_MODE
			case 'X':
				exam_mode = true;
				break;
#endif
			default:
				return E_INVALID_HEADER;
		}
	}

	*machines_number = (uint8_t) magicbytes[2];
	uint32_t expected_size;
	memcpy(&expected_size, magicbytes + 2 + sizeof(uint8_t), sizeof(uint32_t));

	free(magicbytes);

	std::cout << "Opening file created with " << PROGRAM_NAME << " v. " << qMagicbytes.constData() + strlen(PROGRAM_NAME);
	std::cout << "Expected data lenght: " << expected_size << std::endl;
	std::cout << "Reading " << *machines_number << " machines..." << std::endl;
	if(*machines_number <= 0)
		return E_INVALID_FILE;

	char *serialized_data;
	int serialized_data_size;

	if(0);
#ifdef USE_ZLIB
	else if(use_zlib)
	{
		char *z_serialized_data = (char *)malloc(expected_size * sizeof(char));
		uint32_t bytes_read = file.read(z_serialized_data, expected_size);

		if(bytes_read != expected_size)
		{
			free(z_serialized_data);
			return E_INVALID_FILE;
		}

		serialized_data_size = ZlibWrapper::inf(&serialized_data, z_serialized_data, bytes_read);
		free(z_serialized_data);
	}
#endif
#ifdef EXAM_MODE
	else if(exam_mode) //TODO
	{
		return E_UNINMPLEMENTED;
	}
#endif
	else
	{
		serialized_data = (char *)malloc(expected_size * sizeof(char));
		serialized_data_size = file.read(serialized_data, expected_size);

		if(serialized_data_size != expected_size)
			return E_INVALID_FILE;
	}

	uint32_t total_size = 0;
	*settings_header = (settings_header_t *)malloc(*machines_number * sizeof(settings_header_t));
	*settings_ifaces = (settings_iface_t **)malloc(*machines_number * sizeof(settings_iface_t*));

	for(int i = 0; i < *machines_number; i++)
	{
		uint32_t machine_size;
		memcpy(&machine_size, serialized_data+total_size, sizeof(uint32_t));
		memcpy(&(*settings_header)[i], serialized_data+total_size+sizeof(uint32_t), sizeof(settings_header_t));

		(*settings_ifaces)[i] = (settings_iface_t *)malloc((*settings_header)[i].settings_iface_size * sizeof(settings_iface_t));
		memcpy((*settings_ifaces)[i], serialized_data+total_size+sizeof(uint32_t)+sizeof(settings_header_t), (*settings_header)[i].settings_iface_size * sizeof(settings_iface_t));

		total_size += sizeof(uint32_t)+machine_size;
		if(total_size > serialized_data_size)
			return E_INVALID_FILE;
	}

	free(serialized_data);
	return NO_ERROR;
}

bool MachinesDialog::updateMachine(VMTabSettings *vmtab, settings_header_t settings_header, settings_iface_t *settings_ifaces)
{
	uint32_t machineState = vmtab->machine->getState();
	
	if(machineState == MachineState::Running ||
		machineState == MachineState::Paused ||
		machineState == MachineState::Starting)
		return false;

	if(!vmtab->vm->vmSettings->set_machine(settings_header, settings_ifaces))
		return false;
	vmtab->vm->vmSettings->restore();
	vmtab->vm->saveSettings();

	return true;
}

bool MachinesDialog::createMachine(settings_header_t settings_header, settings_iface_t *settings_ifaces)
{
	int newMachine = mainwindow->launchCreateProcess(settings_header.machine_name, true);

	if(newMachine < 0)
		return false;

	if(vmTab_vec->at(newMachine)->setMachineUUID(settings_header.machine_uuid))
	{
		if(!vmTab_vec->at(newMachine)->vm->vmSettings->set_machine(settings_header, settings_ifaces))
			return false;
		vmTab_vec->at(newMachine)->vm->vmSettings->restore();
		vmTab_vec->at(newMachine)->vm->saveSettings();
		return true;	
	}

	return false;
}
