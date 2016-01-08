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

bool VMSettings::save(QString selected_filename)
{
	if(selected_filename.isEmpty())
		selected_filename = fileName;

	char *serializedIfaces = NULL;
	uint32_t size = serialize(&serializedIfaces, savedIfaces);

	serializable_settings_t serializable_settings;
	memset(&serializable_settings, 0, sizeof(serializable_settings_t));
	strcpy(serializable_settings.machine_name, vm->machine->getName().toStdString().c_str());
	strcpy(serializable_settings.machine_uuid, vm->machine->getUUID().toStdString().c_str());
	serializable_settings.serializable_iface_size = savedIfaces_size;

	QFile file(selected_filename);
	
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

bool VMSettings::load(QString selected_filename)
{
	if(selected_filename.isEmpty())
		selected_filename = fileName;

	uint32_t bytes_read;
	char *serializedIfaces;
	serializable_settings_t serializable_settings;

	QFile file(selected_filename);

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

	uint8_t savedIfaces_size = deserialize(&savedIfaces, serializedIfaces, serializable_settings.serializable_iface_size);

	for(int i = 0; i < savedIfaces_size; i++)
		printf("mac@%d: %s\n", i, savedIfaces[i]->mac.toStdString().c_str());
	
	
	return true;
}

uint32_t VMSettings::serialize(char **dest, Iface **src)
{
// 	*dest = (char *)realloc(*dest, savedIfaces_size * sizeof(serializable_iface_t) + sizeof(uint8_t));
// 	memcpy(*dest, &savedIfaces_size, sizeof(uint8_t));
	*dest = (char *)realloc(*dest, savedIfaces_size * sizeof(serializable_iface_t));
	for(int i = 0; i < savedIfaces_size; i++)
	{
		serializable_iface_t serializable_iface = src[i]->getSerializableIface();
// 		memcpy((*dest + sizeof(uint8_t))+(i * sizeof(serializable_iface_t)), &serializable_iface, sizeof(serializable_iface_t));	
		memcpy((*dest)+(i * sizeof(serializable_iface_t)), &serializable_iface, sizeof(serializable_iface_t));	
	}

	return savedIfaces_size * sizeof(serializable_iface_t);
}

uint8_t VMSettings::deserialize(Iface ***dest, char *src, uint8_t size)
{
// 	uint8_t size = (uint8_t) (src[0]);
// 	std::cout << "size: " << (int) size << std::endl;

	serializable_iface_t serializable_iface;
	*dest = (Iface **)realloc(*dest, size * sizeof(Iface*));
	for(int i = 0; i < (int) size; i++)
	{
		memcpy(&serializable_iface, src+(i * sizeof(serializable_iface_t)), sizeof(serializable_iface_t));
		(*dest)[i] = new Iface(serializable_iface);
	}

	return size;
}
