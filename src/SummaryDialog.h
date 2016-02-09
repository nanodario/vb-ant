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

#ifndef SUMMARYDIALOG_H
#define SUMMARYDIALOG_H

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QSpacerItem>
#include <QComboBox>
#include <QCheckBox>

#include "ui_MachinesDialog.h"
#include "VirtualMachine.h"

class Ui_MachinesDialog;
class MainWindow;

class SummaryDialog : public QDialog
{
	Q_OBJECT;

	public:
		SummaryDialog(MainWindow *mainWindow);
		virtual ~SummaryDialog();

	public slots:
		void showByVirtualLan();
		void showByMachine(int vm_index = -1);
		void refresh();
		void populateComboBox();

	private:
		Ui_MachinesDialog *ui;
		QHBoxLayout *radioButtonsLayout;
		QLabel *label;
		QRadioButton *lan_radioButton;
		QSpacerItem *horizontalSpacer;
		QRadioButton *machine_radioButton;
		QComboBox *machine_comboBox;
		QCheckBox *showEmptyNetworks;
		MainWindow *mainWindow;
};

#endif //SUMMARYDIALOG_H
