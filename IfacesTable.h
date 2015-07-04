#ifndef IFACETABLE_H
#define IFACETABLE_H

#include <QBoxLayout>
#include <QCheckBox>
#include <QTableWidget>
#include <QToolButton>
#include <QLineEdit>
#include <QComboBox>
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
#define COLUMN_IFACE_TYPE	5
#define COLUMN_SUBNETNAME	6
#else
#define COLUMN_IFACE_TYPE	3
#define COLUMN_SUBNETNAME	4
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

class AttachmentComboBox : public QWidget
{
	Q_OBJECT
	
	public:
		AttachmentComboBox(QWidget *parent, int row, IfacesTable *destination);
		virtual ~AttachmentComboBox();
		void add_connection();
		void remove_connection();
		QComboBox *comboBox;
	
	public slots:
		void currentIndexChangedSlot(int index);
	
	private:
		int row;
		IfacesTable *destination;
};

class IfacesTable : public QTableWidget
{
	Q_OBJECT
	
	public:
		IfacesTable(QWidget *parent, QBoxLayout *layout, VirtualBoxBridge *vboxbridge, Iface **ifaces);
		~IfacesTable();
#ifdef CONFIGURABLE_IP
		int setIface(int iface, bool enabled = false, QString mac = "", uint32_t attachmentType = NetworkAttachmentType::Null, QString name = "", QString ip = "", QString subnetMask = "", QString subnetName = "");
#else
		int setIface(int iface, bool enabled = false, QString mac = "", uint32_t attachmentType = NetworkAttachmentType::Null, QString name = "", QString subnetName = "");
#endif
		
		bool setStatus(int iface, bool checked);
		bool setName(int iface, QString name);
		bool setMac(int iface, QString mac);
#ifdef CONFIGURABLE_IP
		bool setIp(int iface, QString ip);
		bool setSubnetMask(int iface, QString subnetMask);
#endif
		bool setAttachmentType(int iface, uint32_t attachmentType);
		bool setSubnetName(int iface, QString subnetName);
		void generateMac(int iface);
		
		QStringList getIfaceInfo(int iface);
		void removeIface(int iface);
		Iface *operator[](int iface) const { return ifaces[iface]; };
		
	private slots:
		void cellChangedSlot(int row, int column);
		
	protected:
		Iface **ifaces;
		QTableWidget *ifaces_table;
		
	private:
		VirtualBoxBridge *vboxbridge;
		IMachine *machine;
};

#endif
