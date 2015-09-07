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

#ifndef OSBRIDGE_H
#define OSBRIDGE_H

#include <libkmod.h>
#include <string>

class OSBridge
{
	public:
		static bool loadNbdModule(int devices = 16, int partitions = 0);
		static bool unloadNbdModule();
		static bool checkNbdModule();
		static bool mountVHD(std::string source, std::string target);
		static bool umountVHD(std::string target);
		static bool mountVpartition(std::string source, std::string target, bool readonly = false);
		static bool umountVpartition(std::string target);

	private:
		static int execute_cmd(int argc, char **argv, bool from_path = false);
};

#endif //OSBRIDGE_H
