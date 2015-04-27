#ifndef VMTABSETTINGS_H
#define VMTABSETTINGS_H

#include <QWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include "IfacesTable.h"

class VMTabSettings : public QWidget
{
	Q_OBJECT
	
	public:
		VMTabSettings(QTabWidget *parent, QString _vm_name);
		virtual ~VMTabSettings();
		IfacesTable *ifaces_table;

	private slots:
		void vm_enabledSlot(bool checked);
		void acceptedSlot();
		void rejectedSlot();
		
	private:
		QString vm_name;
		QCheckBox *vm_enabled;
		QWidget *vm_tab;
		QVBoxLayout *verticalLayout;
		QDialogButtonBox *buttonBox;
};

#endif //VMTABSETTINGS_H
