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

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QSpacerItem>
#include <QComboBox>
#include <QCheckBox>

#include "SummaryDialog.h"
#include "MainWindow.h"

#ifdef CONFIGURABLE_IP
	#define VLAN_HEADERS_LABELS "Macchina;Interfaccia;Indirizzo MAC;Indirizzo IP;Subnet mask"
	#define MACHINE_HEADERS_LABELS "Nome rete;Macchina;Interfaccia;Indirizzo MAC;Indirizzo IP;Subnet mask"
#else
	#define VLAN_HEADERS_LABELS "Macchina;Interfaccia;Indirizzo MAC"
	#define MACHINE_HEADERS_LABELS "Nome rete;Macchina;Interfaccia;Indirizzo MAC"
#endif

SummaryDialog::SummaryDialog(MainWindow *mainWindow)
: ui(new Ui_MachinesDialog), mainWindow(mainWindow)
{
	ui->setupUi(this);
	ui->retranslateUi(this);
	ui->verticalLayout->removeWidget(ui->treeWidget);

	radioButtonsLayout = new QHBoxLayout();
	radioButtonsLayout->setObjectName(QString::fromUtf8("radioButtonsLayout"));
	label = new QLabel(this);
	label->setObjectName(QString::fromUtf8("label"));

	radioButtonsLayout->addWidget(label);

	lan_radioButton = new QRadioButton(this);
	lan_radioButton->setObjectName(QString::fromUtf8("lan_radioButton"));
	lan_radioButton->setChecked(true);

	radioButtonsLayout->addWidget(lan_radioButton);

	horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

	radioButtonsLayout->addItem(horizontalSpacer);

	machine_radioButton = new QRadioButton(this);
	machine_radioButton->setObjectName(QString::fromUtf8("machine_radioButton"));

	radioButtonsLayout->addWidget(machine_radioButton);

	machine_comboBox = new QComboBox(this);
	machine_comboBox->setObjectName(QString::fromUtf8("machine_comboBox"));
	machine_comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

	radioButtonsLayout->addWidget(machine_comboBox);

	ui->verticalLayout->addLayout(radioButtonsLayout);

	showEmptyNetworks = new QCheckBox(this);
	showEmptyNetworks->setObjectName(QString::fromUtf8("showEmptyNetworks"));

	ui->verticalLayout->addWidget(showEmptyNetworks);

	connect(lan_radioButton, SIGNAL(toggled(bool)), showEmptyNetworks, SLOT(setEnabled(bool)));
	connect(machine_radioButton, SIGNAL(toggled(bool)), showEmptyNetworks, SLOT(setDisabled(bool)));
	connect(lan_radioButton, SIGNAL(toggled(bool)), machine_comboBox, SLOT(setDisabled(bool)));
	connect(machine_radioButton, SIGNAL(toggled(bool)), machine_comboBox, SLOT(setEnabled(bool)));

	label->setText(QApplication::translate("SummaryDialog", "Visualizza per:", 0, QApplication::UnicodeUTF8));
	lan_radioButton->setText(QApplication::translate("SummaryDialog", "Lan virtuali", 0, QApplication::UnicodeUTF8));
	machine_radioButton->setText(QApplication::translate("SummaryDialog", "Macchina:", 0, QApplication::UnicodeUTF8));
	showEmptyNetworks->setText(QApplication::translate("SummaryDialog", "Mostra reti vuote", 0, QApplication::UnicodeUTF8));

	ui->verticalLayout->addWidget(ui->treeWidget);

	setPalette(mainWindow->palette());

	populateComboBox();

	lan_radioButton->setChecked(true);
	machine_radioButton->setChecked(false);
	machine_comboBox->setEnabled(false);

	connect(lan_radioButton, SIGNAL(toggled(bool)), this, SLOT(showByVirtualLan()));
	connect(showEmptyNetworks, SIGNAL(toggled(bool)), this, SLOT(showByVirtualLan()));
	connect(machine_radioButton, SIGNAL(toggled(bool)), this, SLOT(showByMachine()));
	connect(machine_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(showByMachine(int)));

	showByVirtualLan();
}

SummaryDialog::~SummaryDialog()
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

	delete ui;
}

void SummaryDialog::showByVirtualLan()
{
	ui->treeWidget->clear();

	QStringList headerLabels = QString(VLAN_HEADERS_LABELS).split(";");

	ui->treeWidget->setColumnCount(headerLabels.count());
	ui->treeWidget->setHeaderLabels(headerLabels);

	std::vector<QString> internalNetworks_vec = mainWindow->vboxbridge->getInternalNetworkList();
	std::vector<QTreeWidgetItem*> emptyNetworks;

	for(int internalNetwork_index = 0; internalNetwork_index < internalNetworks_vec.size(); internalNetwork_index++)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setText(0, QString("Nome rete: ").append(internalNetworks_vec.at(internalNetwork_index)));

		for(int machine_index = 0; machine_index < mainWindow->VMTabSettings_vec.size(); machine_index++)
		{
			VirtualMachine *vm = mainWindow->VMTabSettings_vec.at(machine_index)->vm;
			for(int iface_index = 0; iface_index < vm->ifaces_size; iface_index++)
				if(vm->ifaces[iface_index]->attachmentType == NetworkAttachmentType::Internal &&
				   vm->ifaces[iface_index]->attachmentData == internalNetworks_vec.at(internalNetwork_index))
				{
					QTreeWidgetItem *childItem = new QTreeWidgetItem(item);
					childItem->setText(0, vm->machine->getName());
					childItem->setText(1, vm->ifaces[iface_index]->name);
					childItem->setText(2, vm->ifaces[iface_index]->mac);
#ifdef CONFIGURABLE_IP
					childItem->setText(3, vm->ifaces[iface_index]->ip);
					childItem->setText(4, vm->ifaces[iface_index]->subnetMask);
#endif
					if(!vm->ifaces[iface_index]->enabled)
						childItem->setDisabled(true);
				}
		}

		if(item->childCount() > 0)
			ui->treeWidget->addTopLevelItem(item);
		else if(showEmptyNetworks->isChecked())
			emptyNetworks.push_back(item);
	}

	if(showEmptyNetworks->isChecked())
		for(int i = 0; i < emptyNetworks.size(); i++)
			ui->treeWidget->addTopLevelItem(emptyNetworks.at(i));

	ui->treeWidget->expandAll();
	if(ui->treeWidget->topLevelItemCount())
		for(int column = 0; column < headerLabels.size(); column++)
			ui->treeWidget->resizeColumnToContents(column);
}

void SummaryDialog::showByMachine(int vm_index)
{
	if(vm_index < 0)
		vm_index = machine_comboBox->currentIndex();

	ui->treeWidget->clear();

	QStringList headerLabels = QString(MACHINE_HEADERS_LABELS).split(";");

	ui->treeWidget->setColumnCount(headerLabels.count());
	ui->treeWidget->setHeaderLabels(headerLabels);

	VMTabSettings *vmTabSettings = mainWindow->VMTabSettings_vec.at(vm_index);

	for(int iface_index = 0; iface_index < vmTabSettings->vm->ifaces_size; iface_index++)
	{
		if(vmTabSettings->vm->ifaces[iface_index]->attachmentType == NetworkAttachmentType::Internal)	
		{
			QTreeWidgetItem *item = new QTreeWidgetItem();
			item->setText(0, QString("Nome interfaccia: ").append(vmTabSettings->vm->ifaces[iface_index]->name));

			for(int machine_index = 0; machine_index < mainWindow->VMTabSettings_vec.size(); machine_index++)
			{
				VMTabSettings *foreign_vmTabSettings = mainWindow->VMTabSettings_vec.at(machine_index);
				if(machine_index != vm_index)
					for(int foreign_iface_index = 0; foreign_iface_index < vmTabSettings->vm->ifaces_size; foreign_iface_index++)
					{
						if(vmTabSettings->vm->ifaces[iface_index]->attachmentData == foreign_vmTabSettings->vm->ifaces[foreign_iface_index]->attachmentData)
						{
							QTreeWidgetItem *childItem = new QTreeWidgetItem(item);
							childItem->setText(0, vmTabSettings->vm->ifaces[iface_index]->attachmentData);
							childItem->setText(1, foreign_vmTabSettings->vm->machine->getName());
							childItem->setText(2, foreign_vmTabSettings->vm->ifaces[foreign_iface_index]->name);
							childItem->setText(3, foreign_vmTabSettings->vm->ifaces[foreign_iface_index]->mac);
#ifdef CONFIGURABLE_IP
							childItem->setText(4, foreign_vmTabSettings->vm->ifaces[foreign_iface_index]->ip);
							childItem->setText(5, foreign_vmTabSettings->vm->ifaces[foreign_iface_index]->subnetMask);
#endif
							if(!vmTabSettings->vm->ifaces[iface_index]->enabled)
								childItem->setDisabled(true);
						}
					}
			}

			if(item->childCount() > 0)
				ui->treeWidget->addTopLevelItem(item);
		}
	}

	ui->treeWidget->expandAll();
	if(ui->treeWidget->topLevelItemCount())
		for(int column = 0; column < headerLabels.size(); column++)
			ui->treeWidget->resizeColumnToContents(column);
}

void SummaryDialog::refresh()
{
	if(lan_radioButton->isChecked())
		showByVirtualLan();
	else if(machine_radioButton->isChecked())
		showByMachine();
}

void SummaryDialog::populateComboBox()
{
	machine_comboBox->blockSignals(true);
	machine_comboBox->clear();
	for(int machine_index = 0; machine_index < mainWindow->VMTabSettings_vec.size(); machine_index++)
	{
		VirtualMachine *vm = mainWindow->VMTabSettings_vec.at(machine_index)->vm;
		machine_comboBox->addItem(vm->machine->getName());
	}
	machine_comboBox->blockSignals(false);

	refresh();
}
