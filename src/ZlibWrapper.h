/*
 * VB-ANT - VirtualBox - Advanced Network Tool
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

#ifndef ZLIBWRAPPER_H
#define ZLIBWRAPPER_H

#include <QString>
#include <zlib.h>

#define CHUNK 16384

struct simple_list
{
	char data[CHUNK];
	int size;
	struct simple_list *next;
};

typedef struct simple_list simple_list_t;

class ZlibWrapper
{
	public:
		/**
		 * This function deflates __size numbers of bytes pointed by
		 * __src to __dest using zlib compression library at specified
		 * __level.
		 * This function returns size of deflated data.
		 */
		static int def(char **__dest, char *__src, int __size, int __level);

		/**
		 * This function inflates __size numbers of bytes pointed by
		 * __src to __dest using zlib decompression library.
		 * This function returns size of inflated data.
		 */
		static int inf(char **__dest, char *__src, int __size);

		/**
		 * This function returns the zlib version
		 */
		static QString getZlibVersion();
};

#endif //ZLIBWRAPPER_H
