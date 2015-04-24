#ifndef INFODIALOG_H
#define INFODIALOG_H

#include <QDialog>

#include "ui_info_dialog.h"

class Ui_Info_dialog;

class InfoDialog : public QDialog
{
	public:
		InfoDialog();
		virtual ~InfoDialog();
		
	private:
		Ui_Info_dialog *ui;
};

#endif //INFODIALOG_H
