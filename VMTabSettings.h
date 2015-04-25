#ifndef VMTABSETTINGS_H
#define VMTABSETTINGS_H

#include <QWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QDialogButtonBox>
#include "IfacesTable.h"

class VMTabSettings
{
	public:
		VMTabSettings(char *_vm_name);
		virtual ~VMTabSettings();
		void addTo(QTabWidget *parent);
		IfacesTable *ifaces_table;

	private:
		char *vm_name;
		QCheckBox *vm_enabled;
		QWidget *vm_tab;
		QVBoxLayout *verticalLayout;
		QDialogButtonBox *buttonBox;
};

#endif //VMTABSETTINGS_H
