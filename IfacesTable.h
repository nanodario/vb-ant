#ifndef IFACETABLE_H
#define IFACETABLE_H

#include <QBoxLayout>
#include <QTableWidget>
#include <QToolButton>
#include <QLineEdit>
#include <vector>

#include "Iface.h"

#define IFACES_NUMBER 8

#define COLUMN_IFACE_ENABLED	0
#define COLUMN_IFACE_NAME	1
#define COLUMN_MAC		2
#define COLUMN_IP		3
#define COLUMN_SUBNETMASK	4
#define COLUMN_SUBNETNAME	5

class IfacesTable;
class MacWidgetFieldBridge;

class MacWidgetField : public QWidget
{
	public:
		MacWidgetField(QWidget *parent, MacWidgetFieldBridge *bridge);
		virtual ~MacWidgetField();
		void setText(const QString &text);
		QToolButton *button;
		QLineEdit *lineEdit;
};

class MacWidgetFieldBridge : public QWidget
{
	Q_OBJECT

	public:
		MacWidgetFieldBridge(int row, IfacesTable *destination);
		
	public slots:
		void editingFinishedSlot();
		void releasedSlot();
		
	private:
		int row;
		IfacesTable *destination;
};

class IfacesTable : public QTableWidget
{
	Q_OBJECT
	
	public:
		IfacesTable(QWidget *parent, QBoxLayout *layout);
		~IfacesTable();
		int addIface(bool enabled = false, QString name = "", QString mac = "", QString ip = "", QString subnetMask = "", QString subnetName = "");
		
		bool setStatus(int iface, bool checked);
		bool setName(int iface, QString name);
		bool setMac(int iface, QString mac);
		bool setIp(int iface, QString ip);
		bool setSubnetMask(int iface, QString subnetMask);
		bool setSubnetName(int iface, QString subnetName);
		void generateMac(int iface);
		
		QStringList getIfaceInfo(int iface);
		void removeIface(int iface_index);
		
	private slots:
		void cellChangedSlot(int row, int column);
		void cellClickedSlot(int row, int column);
		
	protected:
		std::vector<Iface*> ifaces;
		QTableWidget *ifaces_table;
		
// 	private:
};

#endif
