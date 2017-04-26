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

#include "ExamDialog.h"

ExamDialog::ExamDialog()
: ui(new Ui_ExamDialog)
{
	ui->setupUi(this);
	ui->retranslateUi(this);
	
	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(slotCheckFields()));
}

ExamDialog::~ExamDialog()
{
	delete ui;
}

void ExamDialog::slotCheckFields()
{
	if(ui->lineEditNome->text().trimmed().length() == 0
		|| ui->lineEditCognome->text().trimmed().length() == 0
		|| ui->lineEditMatricola->text().trimmed().length() == 0)
		return;

	ui->lineEditNome->setText(ui->lineEditNome->text().trimmed());
	ui->lineEditCognome->setText(ui->lineEditCognome->text().trimmed());
	ui->lineEditMatricola->setText(ui->lineEditMatricola->text().trimmed());
	accept();
}
