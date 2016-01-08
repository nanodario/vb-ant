/*
 * VB-ANT - VirtualBox . Advanced Network Tool
 * Copyright (C) 2016  Dario Messina
 *
 * This file is part of VB-ANT
 *
 * VB-ANT is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * VB-ANT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "VMSettings.h"
#include <malloc.h>
#include <QFile>

VMSettings::VMSettings(VirtualMachine *vm)
: vm(vm), savedIfaces(NULL), savedIfaces_size(0)
{
	backup();
}

VMSettings::~VMSettings()
{
	for(int i = 0; i < savedIfaces_size; i++)
		free(savedIfaces[i]);
	free(savedIfaces);
}

bool VMSettings::operator==(VMSettings *s)
{

}

void VMSettings::restore()
{
	vm->saveSettings();
	for(int i = 0; i < savedIfaces_size; i++)
	{
		free(vm->ifaces[i]);
		vm->ifaces[i] = savedIfaces[i];
	}

	backup();
}

void VMSettings::backup()
{
	savedIfaces = (Iface **)realloc(savedIfaces, vm->ifaces_size * sizeof(Iface*));
	for(int i = 0; i < vm->ifaces_size; i++)
		savedIfaces[i] = vm->ifaces[i]->copyIface();
	savedIfaces_size = vm->ifaces_size;
}

bool VMSettings::save(QString filename)
{
	char *serializedIfaces = NULL;
	uint32_t size = serialize(&serializedIfaces, savedIfaces);

	QFile file(filename);

	if(file.open(QIODevice::WriteOnly))
	{
		uint32_t bytes_written = file.write(serializedIfaces, size);
		file.close();
		return bytes_written == size;
	}
	else
		return false;
}

bool VMSettings::load(QString filename)
{
	uint32_t bytes_read;
	char *serializedIfaces;

	QFile file(filename);

	if(file.open(QIODevice::ReadOnly))
	{
		uint32_t file_size = file.size();
		serializedIfaces = (char *)malloc(file_size * sizeof(char));

		bytes_read = file.read(serializedIfaces, file_size);
		file.close();
		if(bytes_read != file_size)
			return false;
	}
	else
		return false;

	uint8_t savedIfaces_size = deserialize(&savedIfaces, serializedIfaces);

	return true;
}

uint32_t VMSettings::serialize(char **dest, Iface **src)
{
	*dest = (char *)realloc(*dest, savedIfaces_size * sizeof(serializable_iface_t) + sizeof(uint8_t));
	memcpy(*dest, &savedIfaces_size, sizeof(uint8_t));
	for(int i = 0; i < savedIfaces_size; i++)
	{
		serializable_iface_t serializable_iface = src[i]->getSerializableIface();
		memcpy((*dest + sizeof(uint8_t))+(i * sizeof(serializable_iface_t)), &serializable_iface, sizeof(serializable_iface_t));	
	}

	return savedIfaces_size * sizeof(serializable_iface_t) + sizeof(uint8_t);
}

uint8_t VMSettings::deserialize(Iface ***dest, char *src)
{
	uint8_t size = (uint8_t) (src[0]);
	std::cout << "size: " << (int) size << std::endl;

	*dest = (Iface **)realloc(*dest, size * sizeof(Iface*));
	for(int i = 0; i < (int) size; i++)
	{
		serializable_iface_t serializable_iface;
		memcpy(&serializable_iface, (src + sizeof(uint8_t))+(i * sizeof(serializable_iface_t)), sizeof(serializable_iface_t));
		(*dest)[i] = new Iface(serializable_iface);
	}

	return size;
}

QStringList VMSettings::getSavedIfaceInfo(int iface)
{
	QStringList iface_info;
	iface_info.push_back(QString(savedIfaces[iface]->enabled ? "enabled" : "disabled"));
	iface_info.push_back(savedIfaces[iface]->mac);
	iface_info.push_back(QString(savedIfaces[iface]->cableConnected ? "connected" : "not connected"));
	iface_info.push_back(savedIfaces[iface]->name);
#ifdef CONFIGURABLE_IP
	iface_info.push_back(savedIfaces[iface]->ip);
	iface_info.push_back(savedIfaces[iface]->subnetMask);
#endif
	iface_info.push_back(QString("%1").arg(savedIfaces[iface]->attachmentType));
	iface_info.push_back(savedIfaces[iface]->attachmentData);

	return iface_info;
}

QStringList VMSettings::getCurrentIfaceInfo(int iface)
{
	QStringList iface_info;
	iface_info.push_back(QString(vm->ifaces[iface]->enabled ? "enabled" : "disabled"));
	iface_info.push_back(vm->ifaces[iface]->mac);
	iface_info.push_back(QString(vm->ifaces[iface]->cableConnected ? "connected" : "not connected"));
	iface_info.push_back(vm->ifaces[iface]->name);
#ifdef CONFIGURABLE_IP
	iface_info.push_back(vm->ifaces[iface]->ip);
	iface_info.push_back(vm->ifaces[iface]->subnetMask);
#endif
	iface_info.push_back(QString("%1").arg(vm->ifaces[iface]->attachmentType));
	iface_info.push_back(vm->ifaces[iface]->attachmentData);

	return iface_info;
}

void VMSettings::test()
{
	int j;

	std::cout << "saved_iface_info:" << std::endl;
	for(int i = 0; i < vm->ifaces_size; i++)
	{
		QStringList saved_iface_info = getSavedIfaceInfo(i);
		std::cout << savedIfaces[i] << ": \t";
		for(j = 0; j < saved_iface_info.size() - 1; j++)
			std::cout << saved_iface_info.at(j).toStdString() << " \t";
		std::cout << saved_iface_info.at(j).toStdString() << std::endl;
	}

	std::cout << "current_iface_info:" << std::endl;
	for(int i = 0; i < vm->ifaces_size; i++)
	{
		QStringList current_iface_info = getCurrentIfaceInfo(i);
		std::cout << vm->ifaces[i] << ": \t";
		for(j = 0; j < current_iface_info.size() - 1; j++)
			std::cout << current_iface_info.at(j).toStdString() << " \t";
		std::cout << current_iface_info.at(j).toStdString() << std::endl;
	}
}
