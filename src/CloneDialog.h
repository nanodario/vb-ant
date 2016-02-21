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

#ifndef CLONEDIALOG_H
#define CLONEDIALOG_H

#include <QDialog>

#include "MainWindow.h"
#include "ui_CloneDialog.h"

class CloneDialog : public QDialog
{
	Q_OBJECT

	public:
		CloneDialog(MainWindow *destination, bool newMachine = false);
		virtual ~CloneDialog();
		
	public slots:
		void slotAccepted();
		void slotMachineNameChanged();

	private:
		Ui_clone_machine *ui;
		MainWindow *destination;
		bool newMachine;
		QString machineName;
};

#endif //INFODIALOG_H
