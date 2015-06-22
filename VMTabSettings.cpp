#include "VMTabSettings.h"
#include "VirtualBoxBridge.h"
#include <QTabWidget>

#include <QString>
#include <QStringList>
#include <QWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QHeaderView>

#include <iostream>

VMTabSettings::VMTabSettings(QTabWidget *parent, QString tabname, VirtualBoxBridge *vboxbridge, MachineBridge *machine) : QWidget(parent)
, vm_name(tabname), vboxbridge(vboxbridge), machine(machine), vm(new VirtualMachine(machine))
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

	ifaces_table = new IfacesTable(this, verticalLayout, vboxbridge);
	verticalLayout->addWidget(ifaces_table);
	
	buttonBox = new QDialogButtonBox(this);
	buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
	buttonBox->setStandardButtons(QDialogButtonBox::Apply|QDialogButtonBox::Reset|QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
	
	verticalLayout->addWidget(buttonBox);
	
	connect(vm_enabled, SIGNAL(toggled(bool)), this, SLOT(vm_enabledSlot(bool)));
	connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(clickedSlot(QAbstractButton*)));
	
	ifaces_vec = vm->getIfaces();
	
	int row;
	for(row = 0; row < ifaces_vec.size(); row++)
	{
		bool enabled = ifaces_vec.at(row)->enabled;
		QString name = QString("test%1").arg(row);
		QString mac = ifaces_vec.at(row)->mac;
#ifdef CONFIGURABLE_IP
		QString ip = QString("10.10.10.%1").arg(row);
		QString subnetMask = QString("%1").arg(row);
#endif
		QString subnetName = QString("subnet%1").arg(row);

#ifdef CONFIGURABLE_IP
		ifaces_table->addIface(enabled, mac, name, ip, subnetMask, subnetName);
#else
		ifaces_table->addIface(enabled, mac, name, subnetName);
#endif
	}
}

VMTabSettings::~VMTabSettings()
{
	delete buttonBox;
	delete vm;
	
	delete ifaces_table;
	delete vm_enabled;
	delete verticalLayout;
}

void VMTabSettings::clickedSlot(QAbstractButton *button)
{
	QDialogButtonBox::StandardButton standardButton = buttonBox->standardButton(button);
	switch(standardButton)
	{
		case QDialogButtonBox::Apply:
			std::cout << "apply: \t" << vm_name.toStdString() << std::endl;
			int i;
			for (i = 0; i < 8; i++)
			{
				QStringList iface_info = ifaces_table->getIfaceInfo(i);
				std::cout <<	iface_info.at(0).toStdString() << ", " <<
						iface_info.at(1).toStdString() << ", " <<
						iface_info.at(2).toStdString() << ", " <<
						iface_info.at(3).toStdString() << ", " <<
						iface_info.at(4).toStdString() << std::endl;
				
			}
			break;
		case QDialogButtonBox::Reset:
			std::cout << "reset: \t" << vm_name.toStdString() << std::endl;
			break;
		case QDialogButtonBox::Ok:
			std::cout << "ok: \t" << vm_name.toStdString() << std::endl;
			vm->start();
			break;
		case QDialogButtonBox::Cancel:
			std::cout << "cancel: \t" << vm_name.toStdString() << std::endl;
			vm->stop();
			break;
	}
}

void VMTabSettings::vm_enabledSlot(bool checked)
{
	ifaces_table->setDisabled(!checked);
}
