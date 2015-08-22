/*
 * VBANT - VirtualBox Advanced Network Tool
 * Copyright (C) 2015  Dario Messina
 *
 * This file is part of VBANT
 *
 * VBANT is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * VBANT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

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
#define COLUMN_IFACE_CONNECTED	2
#define COLUMN_IFACE_NAME	3

#ifdef CONFIGURABLE_IP
#define COLUMN_IP		4
#define COLUMN_SUBNETMASK	5
#define COLUMN_IFACE_TYPE	6
#define COLUMN_SUBNETNAME	7
#else
#define COLUMN_IFACE_TYPE	4
#define COLUMN_SUBNETNAME	5
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

class CustomCheckBox : public QWidget
{
	Q_OBJECT
	
	public:
		CustomCheckBox(QWidget *parent, int row, void *destination, bool (*function_forwarder)(void*, int, bool));
		virtual ~CustomCheckBox();
		void setCheckState(Qt::CheckState checked);
		QCheckBox *checkbox;
		
	public slots:
		void toggledSlot(bool checked);
		
	private:
		int row;
		void *destination;
		bool (*function_forwarder)(void*, int, bool);
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
		int setIface(int iface, bool enabled = false, QString mac = "", bool cableConnected = false, uint32_t attachmentType = NetworkAttachmentType::Null, QString subnetName = "", QString name = "", QString ip = "", QString subnetMask = "");
#else
		int setIface(int iface, bool enabled = false, QString mac = "", bool cableConnected = false, uint32_t attachmentType = NetworkAttachmentType::Null, QString subnetName = "", QString name = "");
#endif
		
		bool setStatus(int iface, bool checked);
		bool setName(int iface, QString name);
		bool setMac(int iface, QString mac);
		bool setCableConnected(int iface, bool checked);
		bool setAttachmentType(int iface, uint32_t attachmentType);
		bool setSubnetName(int iface, QString subnetName);
#ifdef CONFIGURABLE_IP
		bool setIp(int iface, QString ip);
		bool setSubnetMask(int iface, QString subnetMask);
#endif
		void generateMac(int iface);
		
		QStringList getIfaceInfo(int iface);
		void removeIface(int iface);
		Iface *operator[](int iface) const { return ifaces[iface]; };
		
		void lockSettings();
		void unlockSettings();

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
