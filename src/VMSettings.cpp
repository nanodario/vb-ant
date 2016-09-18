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
		vm->setSerializableIface(i, savedIfaces[i]);
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
	uint32_t size = serialize(&serializedIfaces, savedIfaces, savedIfaces_size);

	settings_header.settings_iface_size = savedIfaces_size;

	QFile file(selected_filename);
	fileName = selected_filename;

	//Calculate CRC32 of ifaces settings
		
	std::string ifaces_checksum_str = get_ifaces_checksum(&serializedIfaces, savedIfaces_size);
	std::transform(ifaces_checksum_str.begin(), ifaces_checksum_str.end(), ifaces_checksum_str.begin(), toupper_wrapper);

	strcpy(settings_header.ifaces_checksum, ifaces_checksum_str.c_str());
	
	if(file.open(QIODevice::WriteOnly))
	{
		uint32_t bytes_written = file.write("M" SAVEFILE_MAGIC_BYTES);
		bytes_written += file.write((char *)&settings_header, sizeof(settings_header_t));
		bytes_written += file.write(serializedIfaces, size);
		
		file.close();
		return bytes_written == size + sizeof(settings_header_t) + 1 + strlen(SAVEFILE_MAGIC_BYTES);
	}
	else
		return false;
}

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
		QByteArray qMagicbytes = file.readLine();
		if(qMagicbytes.size() < 0 || qMagicbytes.data()[0] != 'M' || strncmp(qMagicbytes.data()+1, SAVEFILE_MAGIC_BYTES, strlen(PROGRAM_NAME)))
			return E_INVALID_FILE;

		std::cout << "Opening file created with " << PROGRAM_NAME << " v. " << qMagicbytes.constData()+1 + strlen(PROGRAM_NAME);
		
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
			std::string ifaces_checksum_str = get_ifaces_checksum(serialized_ifaces, settings_header->settings_iface_size);
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
	savedIfaces_size = deserialize(&savedIfaces, settings_ifaces, settings_header.settings_iface_size);
}

uint32_t VMSettings::get_serializable_machine(settings_header_t *settings_header, char **serialized_ifaces)
{
	memset(settings_header, 0, sizeof(settings_header_t));
	strcpy(settings_header->machine_name, vm->machine->getName().toStdString().c_str());
	strcpy(settings_header->machine_uuid, vm->machine->getUUID().toStdString().c_str());
	settings_header->settings_iface_size = savedIfaces_size;

	uint32_t serialized_ifaces_size = serialize(serialized_ifaces, savedIfaces, savedIfaces_size);
	strcpy(settings_header->ifaces_checksum, get_ifaces_checksum(serialized_ifaces, savedIfaces_size).c_str());	

	return serialized_ifaces_size;
}

std::string VMSettings::get_ifaces_checksum(char **serialized_ifaces, int serialized_ifaces_size)
{
	CRC32 crc32;
	return crc32(*serialized_ifaces, serialized_ifaces_size * sizeof(settings_iface_t));
}

std::string VMSettings::get_ifaces_checksum(settings_header_t settings_header, settings_iface_t *settings_ifaces)
{
	CRC32 crc32;
	return crc32(settings_ifaces, settings_header.settings_iface_size * sizeof(settings_iface_t));
}

bool VMSettings::set_machine(settings_header_t _settings_header, settings_iface_t *_settings_ifaces)
{
	if(get_ifaces_checksum(_settings_header, _settings_ifaces) != _settings_header.ifaces_checksum)
		return false;

	settings_header = _settings_header;
	savedIfaces_size = settings_header.settings_iface_size;
	savedIfaces = (settings_iface_t *)realloc(savedIfaces, savedIfaces_size * sizeof(settings_iface_t));

	memcpy(savedIfaces, _settings_ifaces, savedIfaces_size * sizeof(settings_iface_t));

	return true;
}

uint32_t VMSettings::serialize(char **dest, settings_iface_t *src, uint8_t size)
{
	*dest = (char *)realloc(*dest, size * sizeof(settings_iface_t));
	memset(*dest, 0, size * sizeof(settings_iface_t));
	for(int i = 0; i < size; i++)
		memcpy((*dest)+(i * sizeof(settings_iface_t)), &src[i], sizeof(settings_iface_t));

	return size * sizeof(settings_iface_t);
}

uint8_t VMSettings::deserialize(settings_iface_t **dest, char *src, uint8_t size)
{
	*dest = (settings_iface_t *)realloc(*dest, size * sizeof(settings_iface_t));
	memset(*dest, 0, size * sizeof(settings_iface_t));
	for(int i = 0; i < (int) size; i++)
		memcpy((*dest+i), src+(i * sizeof(settings_iface_t)), sizeof(settings_iface_t));

	return size;
}
