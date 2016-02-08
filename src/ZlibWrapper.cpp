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

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "zlib.h"
#include "ZlibWrapper.h"

int ZlibWrapper::def(char **__dest, char *__src, int __size, int __level)
{
	int ret, flush;
	uint32_t total_size = 0;
	unsigned have;
	z_stream strm;
	unsigned char out[CHUNK];

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit(&strm, __level);
	if (ret != Z_OK)
	{
		printf("[%s] errore: ret = %d\n", __PRETTY_FUNCTION__, ret);
		return ret;
	}

	strm.avail_in = __size;
	strm.next_in = (Bytef *)__src;

	simple_list_t *head_out_data = (simple_list_t *)malloc(sizeof(simple_list_t));
	head_out_data->next = NULL;
	simple_list_t *current_out_data = head_out_data;
	
	/* run deflate() on input until output buffer not full, finish
	 *          compression if all of source has been read in */
	do
	{
		strm.avail_out = CHUNK;
		strm.next_out = out;
		ret = deflate(&strm, Z_FINISH);
		assert(ret != Z_STREAM_ERROR);
		have = CHUNK - strm.avail_out;

		memcpy(current_out_data->data, out, have);
		current_out_data->size = have;

		total_size += have;
		if(strm.avail_out == 0)
		{
			current_out_data->next = (simple_list_t *)malloc(sizeof(simple_list_t));
			current_out_data = current_out_data->next;
			current_out_data->next = NULL;
		}
	} while (strm.avail_out == 0);

	assert(strm.avail_in == 0);

	*__dest = (char *)malloc(sizeof(char) * total_size);
	assert(*__dest != NULL);
	memset(*__dest, 0, total_size);
	current_out_data = head_out_data;
	uint32_t copied_bytes = 0;

	while(current_out_data != NULL)
	{
		memcpy((*__dest) + copied_bytes, current_out_data->data, current_out_data->size);
		copied_bytes += current_out_data->size;
		simple_list_t *old_current_out_data = current_out_data;
		current_out_data = current_out_data->next;
		free(old_current_out_data);
	}

	/* clean up and return */
	(void)deflateEnd(&strm);
	return total_size;
}

int ZlibWrapper::inf(char **__dest, char *__src, int __size)
{
	int ret;
	uint32_t total_size = 0;
	unsigned have;
	z_stream strm;
	unsigned char out[CHUNK];

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);
	if (ret != Z_OK)
	{
		printf("[%s] errore: ret = %d\n", __PRETTY_FUNCTION__, ret);
		return ret;
	}

	simple_list_t *head_out_data = (simple_list_t *)malloc(sizeof(simple_list_t));
	head_out_data->next = NULL;
	simple_list_t *current_out_data = head_out_data;

	strm.avail_in = __size;
	strm.next_in = (Bytef *)__src;
	do {
		strm.avail_out = CHUNK;
		strm.next_out = out;
		ret = inflate(&strm, Z_NO_FLUSH);
		switch (ret)
		{
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;     /* and fall through */
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&strm);
				printf("[%s] errore: ret = %d\n", __PRETTY_FUNCTION__, ret);
				return ret;
		}
		have = CHUNK - strm.avail_out;

		memcpy(current_out_data->data, out, have);
		current_out_data->size = have;
		total_size += have;

		if(strm.avail_out == 0)
		{
			current_out_data->next = (simple_list_t *)malloc(sizeof(simple_list_t));
			current_out_data = current_out_data->next;
			current_out_data->next = NULL;
		}
	} while (strm.avail_out == 0);

	*__dest = (char *)malloc(sizeof(char) * total_size);
	assert(*__dest != NULL);
	memset(*__dest, 0, total_size);
	current_out_data = head_out_data;
	uint32_t copied_bytes = 0;

	while(current_out_data != NULL)
	{
		memcpy((*__dest) + copied_bytes, current_out_data->data, current_out_data->size);
		copied_bytes += current_out_data->size;
		simple_list_t *old_current_out_data = current_out_data;
		current_out_data = current_out_data->next;
		free(old_current_out_data);
	}

	/* clean up and return */
	(void)inflateEnd(&strm);
	return total_size;
}

QString ZlibWrapper::getZlibVersion()
{
	return QString::fromUtf8(zlibVersion());
}
