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

// VB-ANT - VirtualBox - Advanced Network Tool, Un tool per la didattica nel corso di reti di calcolatori

#include <QtGui/QApplication>
#include "MainWindow.h"
#include "OSBridge.h"
#ifdef EXAM_MODE
	#include "ExamDialog.h"
#endif

#ifdef USE_ZLIB
	#include "ZlibWrapper.h"
#endif

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	QStringList args = QApplication::arguments();

	if(OSBridge::checkNbdModule())
		std::cout << "Module nbd loaded" << std::endl;
	else
		std::cout << "Module nbd not loaded" << std::endl;

#ifdef USE_ZLIB
	std::cout << "This program uses zlib version: " << ZlibWrapper::getZlibVersion().toStdString() << std::endl;
#endif
	int retval;
	MainWindow mw((args.count() < 2) ? QString() : args[1]);

#ifdef EXAM_MODE
	ExamDialog *ed = new ExamDialog();
	if(ed->exec() == QDialog::Rejected)
	{
		delete ed;
		retval = 0;
	}
	else
	{
		mw.setExamParams(ed->ui->lineEditNome->text(), ed->ui->lineEditCognome->text(), ed->ui->lineEditMatricola->text());
		mw.show();
		retval = app.exec();
	}
#else
	mw.show();
	retval = app.exec();
#endif
	
	std::cout << "All done." << std::endl;
	return retval;
}
