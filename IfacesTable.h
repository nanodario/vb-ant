#ifndef IFACETABLE_H
#define IFACETABLE_H

#include <QBoxLayout>
#include <QCheckBox>
#include <QTableWidget>
#include <QToolButton>
#include <QLineEdit>
#include <vector>

#include "Iface.h"
#include "VirtualBoxBridge.h"

#define IFACES_NUMBER 8

#define COLUMN_IFACE_ENABLED	0
#define COLUMN_MAC		1
#define COLUMN_IFACE_NAME	2

#ifdef CONFIGURABLE_IP
#define COLUMN_IP		3
#define COLUMN_SUBNETMASK	4
#define COLUMN_SUBNETNAME	5
#else
#define COLUMN_SUBNETNAME	3
#endif

class IfacesTable;
class MacWidgetFieldBridge;

class MacWidgetField : public QWidget
{
	Q_OBJECT
	
	public:
		MacWidgetField(QWidget *parent, int row, IfacesTable *destination);
		virtual ~MacWidgetField();
		void setText(const QString &text);
		QToolButton *button;
		QLineEdit *lineEdit;
		
	public slots:
		void editingFinishedSlot();
		void releasedSlot();
		
	private:
		int row;
		IfacesTable *destination;
};

class IfaceEnableCheckBox : public QWidget
{
	Q_OBJECT
	
	public:
		IfaceEnableCheckBox(QWidget *parent, int row, IfacesTable *destination);
		virtual ~IfaceEnableCheckBox();
		void setCheckState(Qt::CheckState checked);
		QCheckBox *checkbox;
		
	public slots:
		void toggledSlot(bool checked);
		
	private:
		int row;
		IfacesTable *destination;
};

class IfacesTable : public QTableWidget
{
	Q_OBJECT
	
	public:
		IfacesTable(QWidget *parent, QBoxLayout *layout, VirtualBoxBridge *vboxbridge);
		~IfacesTable();
#ifdef CONFIGURABLE_IP
		int addIface(bool enabled = false, QString mac = "", QString name = "", QString ip = "", QString subnetMask = "", QString subnetName = "");
#else
		int addIface(bool enabled = false, QString mac = "", QString name = "", QString subnetName = "");
#endif
		
		bool setStatus(int iface, bool checked);
		bool setName(int iface, QString name);
		bool setMac(int iface, QString mac);
#ifdef CONFIGURABLE_IP
		bool setIp(int iface, QString ip);
		bool setSubnetMask(int iface, QString subnetMask);
#endif
		bool setSubnetName(int iface, QString subnetName);
		void generateMac(int iface);
		
		QStringList getIfaceInfo(int iface);
		void removeIface(int iface_index);
		
	private slots:
		void cellChangedSlot(int row, int column);
		
	protected:
		std::vector<Iface*> ifaces;
		QTableWidget *ifaces_table;
		
	private:
		VirtualBoxBridge *vboxbridge;
		IMachine *machine;
};

#endif
