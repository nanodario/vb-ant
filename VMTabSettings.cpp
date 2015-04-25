#include "VMTabSettings.h"
#include <QTabWidget>

#include <QWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QHeaderView>

VMTabSettings::VMTabSettings(char *tabname)
: vm_name(tabname)
{
	vm_tab = new QWidget();
	vm_tab->setObjectName(QString::fromUtf8(tabname));

	verticalLayout = new QVBoxLayout(vm_tab);
	verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));

	vm_enabled = new QCheckBox(vm_tab);
	vm_enabled->setText(QString::fromUtf8("Abilita macchina virtuale"));
	vm_enabled->setChecked(true);
	vm_enabled->setEnabled(true);
	vm_enabled->setObjectName(QString::fromUtf8("vm_enabled"));
	verticalLayout->addWidget(vm_enabled);

	ifaces_table = new IfacesTable(vm_tab, verticalLayout);

	buttonBox = new QDialogButtonBox(vm_tab);
	buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
	buttonBox->setStandardButtons(QDialogButtonBox::Apply|QDialogButtonBox::Reset);
	
	verticalLayout->addWidget(buttonBox);
}

void VMTabSettings::addTo(QTabWidget *parent)
{
	parent->addTab(vm_tab, vm_name);
}

VMTabSettings::~VMTabSettings()
{
	delete verticalLayout;
	delete buttonBox;
	delete ifaces_table;
	delete vm_tab;
}


