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
: fileName(""), vm(vm), savedIfaces(NULL), savedIfaces_size(0)
{
	memset(&serializable_settings, 0, sizeof(serializable_settings_t));
	strcpy(serializable_settings.machine_name, vm->machine->getName().toStdString().c_str());
	strcpy(serializable_settings.machine_uuid, vm->machine->getUUID().toStdString().c_str());
	backup();
}

VMSettings::~VMSettings()
{
	free(savedIfaces);
}

bool VMSettings::operator==(VMSettings *s)
{

}

void VMSettings::restore()
{
	for(int i = 0; i < savedIfaces_size; i++)
	{
		if(vm->ifaces[i] != NULL)
			vm->restoreSerializableIface(i, savedIfaces[i]);
		else
			vm->ifaces[i] = new Iface(savedIfaces[i]);
	}
	
}

void VMSettings::backup()
{
	savedIfaces = (serializable_iface_t *)realloc(savedIfaces, vm->ifaces_size * sizeof(serializable_iface_t));
	for(int i = 0; i < vm->ifaces_size; i++)
		savedIfaces[i] = vm->ifaces[i]->getSerializableIface();
	savedIfaces_size = vm->ifaces_size;
}

bool VMSettings::save(QString selected_filename)
{
	if(selected_filename.isEmpty())
		selected_filename = fileName;

	char *serializedIfaces = NULL;
	uint32_t size = serialize(&serializedIfaces);

	serializable_settings.serializable_iface_size = savedIfaces_size;

	QFile file(selected_filename);
	fileName = selected_filename;
	
	if(file.open(QIODevice::WriteOnly))
	{
		uint32_t bytes_written = file.write((char *)&serializable_settings, sizeof(serializable_settings_t));
		bytes_written += file.write(serializedIfaces, size);
		
		file.close();
		return bytes_written == size + sizeof(serializable_settings_t);
	}
	else
		return false;
}

serializable_settings_t VMSettings::read(QString selected_filename)
{
	if(selected_filename.isEmpty())
		selected_filename = fileName;

	uint32_t bytes_read;
	serializable_settings_t serializable_settings_tmp;

	QFile file(selected_filename);
	fileName = selected_filename;

	if(file.open(QIODevice::ReadOnly))
	{
		bytes_read = file.read((char *)&serializable_settings_tmp, sizeof(serializable_settings_t));

		file.close();
		if(bytes_read == sizeof(serializable_settings_t))
			return serializable_settings_tmp;
	}

	memset(&serializable_settings_tmp, 0, sizeof(serializable_settings_t));
	return serializable_settings_tmp;
}

bool VMSettings::load(QString selected_filename)
{
	if(selected_filename.isEmpty())
		selected_filename = fileName;

	uint32_t bytes_read;
	char *serializedIfaces;

	QFile file(selected_filename);
	fileName = selected_filename;
	
	if(file.open(QIODevice::ReadOnly))
	{
		uint32_t file_size = file.size();
		bytes_read = file.read((char *)&serializable_settings, sizeof(serializable_settings_t));
		uint32_t serializable_iface_size = serializable_settings.serializable_iface_size * sizeof(serializable_iface_t);

		serializedIfaces = (char *)malloc(serializable_iface_size * sizeof(char));
		bytes_read += file.read(serializedIfaces, serializable_iface_size);

		file.close();
		if(bytes_read != file_size)
			return false;
	}
	else
		return false;

	uint8_t savedIfaces_size = deserialize(serializedIfaces, serializable_settings.serializable_iface_size);

	return true;
}

uint32_t VMSettings::serialize(char **dest)
{
	*dest = (char *)realloc(*dest, savedIfaces_size * sizeof(serializable_iface_t));
	for(int i = 0; i < savedIfaces_size; i++)
		memcpy((*dest)+(i * sizeof(serializable_iface_t)), &savedIfaces[i], sizeof(serializable_iface_t));	

	return savedIfaces_size * sizeof(serializable_iface_t);
}

uint8_t VMSettings::deserialize(char *src, uint8_t size)
{
	savedIfaces = (serializable_iface_t *)realloc(savedIfaces, size * sizeof(serializable_iface_t));
	for(int i = 0; i < (int) size; i++)
		memcpy(&(savedIfaces[i]), src+(i * sizeof(serializable_iface_t)), sizeof(serializable_iface_t));

	return size;
}
