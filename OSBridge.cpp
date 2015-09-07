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

#include "OSBridge.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <libkmod.h>
#include <sys/mount.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>

#define GRAPHIC_SUDO "kdesudo"
#define NBDTOOL_CMD_NAME "nbdtool"

int OSBridge::execute_cmd(int argc, char **argv, bool from_path)
{
#ifdef DEBUG_FLAG
	std::cout << "*** Executing:";
	for(int i = 0; i < argc - 1; i++)
		std::cout << " " << argv[i];
	std::cout << std::endl;
#endif
	
	pid_t pid;
	int status;
	
	if((pid = fork()) < 0)
	{
		std::cerr << "*** ERROR: fork() failed" << std::endl;
		return 1;
	}
	else if(pid == 0)
	{
		if(from_path)
			if(execvp(*argv, argv) < 0)
				std::cerr << "*** ERROR: execvp() failed" << std::endl;
		else
			if(execv(*argv, argv) < 0)
				std::cerr << "*** ERROR: execv() failed" << std::endl;
		exit(2);
	}
	else
	{
		while(wait(&status) != pid);
		std::cout << "*** Child process (pid: " << pid << ", ppid: " << getppid() << ") terminated. Return value: " << status << std::endl;
	}
	
	return status;
}

bool OSBridge::checkNbdModule()
{
	char *argv[3];
	argv[0] = (char *)malloc(sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 3)); strcpy(argv[0], "./" NBDTOOL_CMD_NAME);
	argv[1] = (char *)malloc(sizeof(char) * (strlen("check") + 1)); strcpy(argv[1], "check");
	argv[2] = NULL;
	
	return execute_cmd(3, argv, true) == 0;
}

bool OSBridge::loadNbdModule(int devices, int partitions)
{
	std::stringstream devices_ss;    devices_ss << devices;
	std::stringstream partitions_ss; partitions_ss << partitions;

	char *argv[4];
	argv[0] = (char *)malloc(sizeof(char) * (strlen(GRAPHIC_SUDO) + 1)); strcpy(argv[0], GRAPHIC_SUDO);
	argv[1] = (char *)malloc(sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 1)); strcpy(argv[1], NBDTOOL_CMD_NAME);
	argv[2] = (char *)malloc(sizeof(char) * (strlen("load") + 1)); strcpy(argv[2], "load");
	argv[3] = (char *)malloc(sizeof(char) * (devices_ss.str().length() + 1)); strcpy(argv[3], devices_ss.str().c_str());
	argv[4] = (char *)malloc(sizeof(char) * (partitions_ss.str().length() + 1)); strcpy(argv[4], partitions_ss.str().c_str());
	argv[5] = NULL;
	
	int retval = execute_cmd(6, argv);
	if(retval != 0)
	{
		argv[1] = (char *)realloc(argv[1], sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 3)); strcpy(argv[1], "./" NBDTOOL_CMD_NAME);
		retval = execute_cmd(6, argv, true);
	}

	return retval == 0;
}

bool OSBridge::unloadNbdModule()
{
	char *argv[4];
	argv[0] = (char *)malloc(sizeof(char) * (strlen(GRAPHIC_SUDO) + 1)); strcpy(argv[0], GRAPHIC_SUDO);
	argv[1] = (char *)malloc(sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 1)); strcpy(argv[1], NBDTOOL_CMD_NAME);
	argv[2] = (char *)malloc(sizeof(char) * (strlen("unload") + 1)); strcpy(argv[2], "unload");
	argv[3] = NULL;
	
	int retval = execute_cmd(4, argv);
	if(retval != 0)
	{
		argv[1] = (char *)realloc(argv[1], sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 3)); strcpy(argv[1], "./" NBDTOOL_CMD_NAME);
		retval = execute_cmd(4, argv, true);
	}
	
	return retval == 0;
}

bool OSBridge::mountVHD(std::string source, std::string target)
{
	char *argv[6];
	argv[0] = (char *)malloc(sizeof(char) * (strlen(GRAPHIC_SUDO) + 1)); strcpy(argv[0], GRAPHIC_SUDO);
	argv[1] = (char *)malloc(sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 1)); strcpy(argv[1], NBDTOOL_CMD_NAME);
	argv[2] = (char *)malloc(sizeof(char) * (strlen("mountVHD") + 1)); strcpy(argv[2], "mountVHD");
	argv[3] = (char *)malloc(sizeof(char) * (source.length() + 1)); strcpy(argv[3], source.c_str());
	argv[4] = (char *)malloc(sizeof(char) * (target.length() + 1)); strcpy(argv[4], target.c_str());
	argv[5] = NULL;
	
	int retval = execute_cmd(6, argv);
	if(retval != 0)
	{
		argv[1] = (char *)realloc(argv[1], sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 3)); strcpy(argv[1], "./" NBDTOOL_CMD_NAME);
		retval = execute_cmd(6, argv, true);
	}
	
	return retval == 0;
}

bool OSBridge::umountVHD(std::string target)
{
	char *argv[5];
	argv[0] = (char *)malloc(sizeof(char) * (strlen(GRAPHIC_SUDO) + 1)); strcpy(argv[0], GRAPHIC_SUDO);
	argv[1] = (char *)malloc(sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 1)); strcpy(argv[1], NBDTOOL_CMD_NAME);
	argv[2] = (char *)malloc(sizeof(char) * (strlen("umountVHD") + 1)); strcpy(argv[2], "umountVHD");
	argv[3] = (char *)malloc(sizeof(char) * (target.length() + 1)); strcpy(argv[3], target.c_str());
	argv[4] = NULL;
	
	int retval = execute_cmd(5, argv);
	if(retval != 0)
	{
		argv[1] = (char *)realloc(argv[1], sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 3)); strcpy(argv[1], "./" NBDTOOL_CMD_NAME);
		retval = execute_cmd(5, argv, true);
	}
	
	return retval == 0;
}

bool OSBridge::mountVpartition(std::string source, std::string target, bool readonly)
{
	struct stat s;
	if(stat(target.c_str(), &s) < 0 && errno == ENOENT)
		mkdir(target.c_str(), 0777);

	char *argv[7];
	argv[0] = (char *)malloc(sizeof(char) * (strlen(GRAPHIC_SUDO) + 1)); strcpy(argv[0], GRAPHIC_SUDO);
	argv[1] = (char *)malloc(sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 1)); strcpy(argv[1], NBDTOOL_CMD_NAME);
	argv[2] = (char *)malloc(sizeof(char) * (strlen("mount") + 1)); strcpy(argv[2], "mount");
	argv[3] = (char *)malloc(sizeof(char) * (source.length() + 1)); strcpy(argv[3], source.c_str());
	argv[4] = (char *)malloc(sizeof(char) * (target.length() + 1)); strcpy(argv[4], target.c_str());
	argv[5] = (char *)malloc(sizeof(char) * (3));

	if(readonly)
		strcpy(argv[5], "ro");
	else
		strcpy(argv[5], "rw");

	argv[6] = NULL;
	
	int retval = execute_cmd(7, argv);
	if(retval != 0)
	{
		argv[1] = (char *)realloc(argv[1], sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 3)); strcpy(argv[1], "./" NBDTOOL_CMD_NAME);
		retval = execute_cmd(7, argv, true);
	}
	
	return retval == 0;
}

bool OSBridge::umountVpartition(std::string target)
{
	char *argv[5];
	argv[0] = (char *)malloc(sizeof(char) * (strlen(GRAPHIC_SUDO) + 1)); strcpy(argv[0], GRAPHIC_SUDO);
	argv[1] = (char *)malloc(sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 1)); strcpy(argv[1], NBDTOOL_CMD_NAME);
	argv[2] = (char *)malloc(sizeof(char) * (strlen("umount") + 1)); strcpy(argv[2], "umount");
	argv[3] = (char *)malloc(sizeof(char) * (target.length() + 1)); strcpy(argv[3], target.c_str());
	argv[4] = NULL;
	
	int retval = execute_cmd(5, argv);
	if(retval != 0)
	{
		argv[1] = (char *)realloc(argv[1], sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 3)); strcpy(argv[1], "./" NBDTOOL_CMD_NAME);
		retval = execute_cmd(5, argv, true);
	}
	
	if(retval == 0)
	{
		struct stat s;
		if(stat(target.c_str(), &s) >= 0 && ((s.st_mode & S_IFMT) == S_IFDIR))
			return rmdir(target.c_str()) == 0;
	}
	
	return retval == 0;
}

