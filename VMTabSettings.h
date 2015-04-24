#ifndef VMTABSETTINGS_H
#define VMTABSETTINGS_H

#include <QWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QTableView>
#include <QDialogButtonBox>

class VMTabSettings
{
	public:
		VMTabSettings(char *_vm_name);
		virtual ~VMTabSettings();
		void addTo(QTabWidget *parent);

	private:
		char *vm_name;
		QCheckBox *vm_enabled;
		QWidget *vm_tab;
		QVBoxLayout *verticalLayout;
		QTableView *ifaces_table;
		QDialogButtonBox *buttonBox;
};

#endif //VMTABSETTINGS_H
