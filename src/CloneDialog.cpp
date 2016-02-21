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

#include "CloneDialog.h"

CloneDialog::CloneDialog(MainWindow *destination, bool newMachine)
: ui(new Ui_clone_machine), destination(destination), newMachine(newMachine), machineName(QString::fromUtf8(""))
{
	ui->setupUi(this);
	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(slotAccepted()));

	ui->retranslateUi(this);
	for(int i = 0; i < ui->buttonBox->buttons().size(); i++)
	{
		switch(ui->buttonBox->standardButton(ui->buttonBox->buttons()[i]))
		{
			case QDialogButtonBox::Ok: ui->buttonBox->buttons()[i]->setText("Ok"); break;
			case QDialogButtonBox::Cancel: ui->buttonBox->buttons()[i]->setText("Annulla"); break;
		}
	}

	if(newMachine)
	{
		setWindowTitle(QApplication::translate("clone_machine", "Crea macchina...", 0, QApplication::UnicodeUTF8));
		ui->checkBox->setChecked(true);
		ui->checkBox->setEnabled(false);
	}

	connect(ui->lineEdit, SIGNAL(editingFinished()), this, SLOT(slotMachineNameChanged()));
}

CloneDialog::~CloneDialog()
{
	delete ui;
}

void CloneDialog::slotAccepted()
{
	if(machineName.length() > 0)
	{
		if(newMachine)
			destination->launchCreateProcess(machineName, ui->checkBox->isChecked());
		else
			destination->launchCloneProcess(machineName, ui->checkBox->isChecked());
	}
}

void CloneDialog::slotMachineNameChanged()
{
	machineName = destination->vboxbridge->validateMachineName(ui->lineEdit->text(), destination->machines_vec.size());
	ui->lineEdit->setText(machineName);
}
