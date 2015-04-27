#include "VMTabSettings.h"
#include <QTabWidget>

#include <QString>
#include <QWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QHeaderView>

#include <iostream>

VMTabSettings::VMTabSettings(QTabWidget *parent, QString tabname) : QWidget(parent)
, vm_name(tabname)
{
	setObjectName(tabname);

	verticalLayout = new QVBoxLayout(this);
	verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));

	vm_enabled = new QCheckBox(this);
	vm_enabled->setText(QString::fromUtf8("Abilita macchina virtuale"));
	vm_enabled->setChecked(true);
	vm_enabled->setEnabled(true);
	vm_enabled->setObjectName(QString::fromUtf8("vm_enabled"));
	verticalLayout->addWidget(vm_enabled);

	ifaces_table = new IfacesTable(this, verticalLayout);
	verticalLayout->addWidget(ifaces_table);
	
	buttonBox = new QDialogButtonBox(this);
	buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
	buttonBox->setStandardButtons(QDialogButtonBox::Apply|QDialogButtonBox::Reset);
	
	connect(vm_enabled, SIGNAL(toggled(bool)), this, SLOT(vm_enabledSlot(bool)));
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(acceptedSlot()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(rejectedSlot()));
	
	verticalLayout->addWidget(buttonBox);
}

void VMTabSettings::vm_enabledSlot(bool checked)
{
	ifaces_table->setDisabled(!checked);
}

void VMTabSettings::acceptedSlot()
{
	std::cout << "Apply!" << std::endl;
}

void VMTabSettings::rejectedSlot()
{
	std::cout << "Reset!" << std::endl;
}

VMTabSettings::~VMTabSettings()
{
	delete verticalLayout;
	delete buttonBox;
	delete ifaces_table;
// 	delete vm_tab;
}


