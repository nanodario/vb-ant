#ifndef VMTABSETTINGS_H
#define VMTABSETTINGS_H

#include <vector>
#include <QWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include "IfacesTable.h"
#include "VirtualMachine.h"
#include "VirtualBoxBridge.h"

class VMTabSettings : public QWidget
{
	Q_OBJECT
	
	public:
		VMTabSettings(QTabWidget *parent, QString _vm_name, VirtualBoxBridge *vboxbridge, MachineBridge *machine);
		virtual ~VMTabSettings();
		IfacesTable *ifaces_table;
		
	private slots:
		void clickedSlot(QAbstractButton*);
		void vm_enabledSlot(bool checked);
		
	private:
		QString vm_name;
		QCheckBox *vm_enabled;
		QWidget *vm_tab;
		QVBoxLayout *verticalLayout;
		QDialogButtonBox *buttonBox;
		VirtualMachine *vm;
		std::vector<Iface*> ifaces_vec;
		VirtualBoxBridge *vboxbridge;
		MachineBridge *machine;
};

#endif //VMTABSETTINGS_H
