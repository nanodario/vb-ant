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

#ifndef VMSETTINGS_H
#define VMSETTINGS_H

#define PROGRAM_VERSION "0.1"
#define SAVEFILE_MAGIC_BYTES PROGRAM_NAME PROGRAM_VERSION "\n"

#include "VirtualMachine.h"
#include "crc32.h"

/**
 * Settings format for machine save file:
 * save file magic bytes:
 * magic bytes:
 * 	   'M'				[   1 B]
 * 	   SAVEFILE_MAGIC_BYTES		[variable lenght]
 * --- new line ---			[   1 B]
 * header [2059 B]:
 * 	   machine name 		[1024 B]
 * 	   machine uuid 		[1024 B]
 * 	   ifaces checksum 		[  10 B]
 * 	   number of ifaces 		[   1 B]
 * ifaces #1 data (each iface) [5186 B]:
 * 	   last valid iface name 	[1024 B]
 * 	   current iface name 		[1024 B]
 * 	   iface mac 			[  60 B]
 * 	   iface attachmentData 	[1024 B]
 * 	   iface IP 			[1024 B]
 * 	   iface subnet mask 		[1024 B]
 * 	   iface attachment type 	[   4 B]
 * 	   iface enabled flag 		[   1 B]
 * 	   iface cable connected flag 	[   1 B]
 * ifaces #2 data (each iface) [5186 B]
 * ifaces #n data (each iface) [5186 B]
 *
 * A machine with 8 iface has a 43558 B settings file if SAVEFILE_MAGIC_BYTES is "vb-ant0.1"
 */

typedef struct
{
	char machine_name[1024];
	char machine_uuid[1024];
	char ifaces_checksum[10];
	uint8_t settings_iface_size;
} settings_header_t;

typedef enum
{
        NO_ERROR,
        E_INVALID_HEADER,
        E_INVALID_FILE,
        E_MACHINE_MISMATCH,
        E_INVALID_CHECKSUM,
	E_UNINMPLEMENTED,
        E_UNKNOWN
} read_result_t;

class MachinesDialog;

class VMSettings
{
	friend class MachinesDialog;

	public:
		VMSettings(VirtualMachine *vm);
		~VMSettings();

		void backup();
		void restore();

		bool save(QString selected_filename = "");
		read_result_t read(settings_header_t *settings_header, char **settings_ifaces, QString selected_filename);
		void load(settings_header_t settings_header, char *settings_ifaces);

		bool operator==(VMSettings *s);
		QString fileName;
		settings_header_t settings_header;
		static std::string get_ifaces_checksum(char **serialized_ifaces, int serialized_ifaces_size);
		static std::string get_ifaces_checksum(settings_header_t settings_header, settings_iface_t *settings_ifaces);
		bool set_machine(settings_header_t settings_header, settings_iface_t *settings_ifaces);

		/**
		 * Allocate SIZE * sizeof(settings_iface_t) bytes in DEST and copies
		 * SIZE * sizeof(settings_iface_t) bytes memory of SRC to DEST
		 */
		static uint32_t serialize(char **dest, settings_iface_t *src, uint8_t size);

		/**
		 * Allocate SIZE * sizeof(settings_iface_t) bytes in DEST and copies
		 * SIZE * sizeof(settings_iface_t) bytes memory of SRC to DEST
		 */
		static uint8_t deserialize(settings_iface_t **dest, char *src, uint8_t size);

	private:
		uint32_t get_serializable_machine(settings_header_t *settings_header, char **settings_ifaces);

		VirtualMachine *vm;
		settings_iface_t *savedIfaces;
		uint8_t savedIfaces_size;
};

#endif //VMSETTINGS_H
