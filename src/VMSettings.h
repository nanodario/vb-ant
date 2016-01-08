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

#include "VirtualMachine.h"

/**
 * Settings format for machine save file:
 * header [2045 B]:
 * 	   machine name 		[1024 B]
 * 	   machine uuid 		[1024 B]
 * 	   number of ifaces 		[   1 B]
 * ifaces #1 data (each iface) [3138 B or 5186 B]:
 * 	   last valid iface name 	[1024 B]
 * 	   current iface name 		[1024 B]
 * 	   iface mac 			[  60 B]
 * 	   iface attachmentData 	[1024 B]
 * 	#ifdef CONFIGURABLE_IP
 * 	   iface IP 			[1024 B]
 * 	   iface subnet mask 		[1024 B]
 * 	#endif
 * 	   iface attachment type 	[   4 B]
 * 	   iface enabled flag 		[   1 B]
 * 	   iface cable connected flag 	[   1 B]
 * ifaces #2 data (each iface) [3138 B or 5186 B]
 * ifaces #n data (each iface) [3138 B or 5186 B]
 *
 * A machine with 8 iface has a 43533 B (or 27149 B without IP support) settings file
 */

typedef struct
{
	char machine_name[1024];
	char machine_uuid[1024];
	uint8_t serializable_iface_size;
} serializable_settings_t;

class VMSettings
{
	public:
		VMSettings(VirtualMachine *vm);
		~VMSettings();

		void backup();
		void restore();

		bool save(QString selected_filename = "");
		bool load(QString selected_filename = "");

		bool operator==(VMSettings *s);
		QString fileName;
	private:
		uint32_t serialize(char **dest, Iface **src);
		uint8_t deserialize(Iface ***dest, char *src, uint8_t size);

		VirtualMachine *vm;
		Iface **savedIfaces;
		uint8_t savedIfaces_size;
};

#endif //VMSETTINGS_H
