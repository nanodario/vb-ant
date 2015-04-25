#ifndef IFACETABLE_H
#define IFACETABLE_H

#include <QBoxLayout>
#include <QTableWidget>
#include <vector>

#include "Iface.h"

class IfacesTable
{
	public:
		IfacesTable(QWidget *parent, QBoxLayout *layout);
		~IfacesTable();
		int addIface(QString name);
		int addIface(QString name, QString mac);
		void removeIface(int iface_index);
		
	protected:
		QTableWidget *ifaces_table;
		
	private:
		std::vector<Iface*> ifaces;
};

#endif
