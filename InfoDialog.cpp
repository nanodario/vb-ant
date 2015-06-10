#include "InfoDialog.h"

InfoDialog::InfoDialog()
: ui(new Ui_Info_dialog)
{
	ui->setupUi(this);
}

InfoDialog::~InfoDialog()
{
	delete ui;
}
