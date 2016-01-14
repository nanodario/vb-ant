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
#include "crc32.h"
#include <malloc.h>
#include <QFile>

static int toupper_wrapper(int c)
{
	return std::toupper((unsigned char) c);
}

VMSettings::VMSettings(VirtualMachine *vm)
: fileName(""), vm(vm), savedIfaces(NULL), savedIfaces_size(0)
{
	memset(&settings_header, 0, sizeof(settings_header_t));
	strcpy(settings_header.machine_name, vm->machine->getName().toStdString().c_str());
	strcpy(settings_header.machine_uuid, vm->machine->getUUID().toStdString().c_str());
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
	savedIfaces = (settings_iface_t *)realloc(savedIfaces, vm->ifaces_size * sizeof(settings_iface_t));
	for(int i = 0; i < vm->ifaces_size; i++)
		savedIfaces[i] = vm->ifaces[i]->getSerializableIface();
	savedIfaces_size = vm->ifaces_size;
}

bool VMSettings::save(QString selected_filename)
{
	if(selected_filename.isEmpty())
		selected_filename = fileName;

	CRC32 crc32;
	char *serializedIfaces = NULL;
	uint32_t size = serialize(&serializedIfaces);

	settings_header.settings_iface_size = savedIfaces_size;

	QFile file(selected_filename);
	fileName = selected_filename;

	//Calculate CRC32 of ifaces settings
		
	std::string ifaces_checksum_str = crc32(serializedIfaces, savedIfaces_size * sizeof(settings_iface_t));
	std::transform(ifaces_checksum_str.begin(), ifaces_checksum_str.end(), ifaces_checksum_str.begin(), toupper_wrapper);

	strcpy(settings_header.ifaces_checksum, ifaces_checksum_str.c_str());
	
	if(file.open(QIODevice::WriteOnly))
	{
		uint32_t bytes_written = file.write((char *)&settings_header, sizeof(settings_header_t));
		bytes_written += file.write(serializedIfaces, size);
		
		file.close();
		return bytes_written == size + sizeof(settings_header_t);
	}
	else
		return false;
}

//TEST
read_result_t VMSettings::read(settings_header_t *settings_header, char **serialized_ifaces, QString selected_filename)
{
	if(selected_filename.isEmpty())
		selected_filename = fileName;

	fileName = selected_filename;
	uint32_t bytes_read;

	QFile file(selected_filename);

	if(file.open(QIODevice::ReadOnly))
	{
		//read header
		bytes_read = file.read((char *)settings_header, sizeof(settings_header_t));
		uint32_t settings_iface_size = settings_header->settings_iface_size * sizeof(settings_iface_t);
		if(bytes_read != sizeof(settings_header_t))
			return E_INVALID_HEADER;

		//read ifaces data
		*serialized_ifaces = (char *)malloc(settings_iface_size * sizeof(char));
		bytes_read = file.read((char*) *serialized_ifaces, settings_iface_size);
		

		file.close();
		if(bytes_read == settings_iface_size)
		{
			CRC32 crc32;

			std::string ifaces_checksum_str = crc32(*serialized_ifaces, settings_header->settings_iface_size * sizeof(settings_iface_t));
			std::transform(ifaces_checksum_str.begin(), ifaces_checksum_str.end(), ifaces_checksum_str.begin(), toupper_wrapper);

			if(strcmp(settings_header->ifaces_checksum, ifaces_checksum_str.c_str()))
				return E_INVALID_CHECKSUM;
			if(strcmp(settings_header->machine_uuid, vm->machine->getUUID().toStdString().c_str()))
				return E_MACHINE_MISMATCH;
			return NO_ERROR;
		}
		return E_INVALID_FILE;	
	}

	return E_UNKNOWN;
}

void VMSettings::load(settings_header_t settings_header, char *settings_ifaces)
{
	savedIfaces_size = deserialize(settings_ifaces, settings_header.settings_iface_size);
}

uint32_t VMSettings::serialize(char **dest)
{
	*dest = (char *)realloc(*dest, savedIfaces_size * sizeof(settings_iface_t));
	for(int i = 0; i < savedIfaces_size; i++)
		memcpy((*dest)+(i * sizeof(settings_iface_t)), &savedIfaces[i], sizeof(settings_iface_t));	

	return savedIfaces_size * sizeof(settings_iface_t);
}

uint8_t VMSettings::deserialize(char *src, uint8_t size)
{
	savedIfaces = (settings_iface_t *)realloc(savedIfaces, size * sizeof(settings_iface_t));
	for(int i = 0; i < (int) size; i++)
		memcpy(&(savedIfaces[i]), src+(i * sizeof(settings_iface_t)), sizeof(settings_iface_t));

	return size;
}
