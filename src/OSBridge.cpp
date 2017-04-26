/*
 * VB-ANT - VirtualBox - Advanced Network Tool
 * Copyright (C) 2015, 2017  Dario Messina
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

#include "OSBridge.h"
#include "MainWindow.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
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
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QAbstractButton>

// #define USE_SUDO
#define GRAPHIC_SUDO "kdesudo"
#define NBDTOOL_CMD_NAME "nbdtool"

int OSBridge::execute_cmd(int argc, char **argv, bool from_path)
{
	char **argv_cmd = argv;
	int argc_cmd = argc;

#ifdef USE_SUDO
	char **argv_with_sudo = (char **)malloc(sizeof(char*) * argc + 1);
	argv_with_sudo[0] = (char *)malloc(sizeof(char) * (strlen(GRAPHIC_SUDO) + 1)); strcpy(argv_with_sudo[0], GRAPHIC_SUDO);
	for(int i = 0; i < argc; i++)
		argv_with_sudo[i+1] = argv[i];
	argv_cmd = argv_with_sudo;
	argc_cmd = argc + 1;
#endif

#ifdef DEBUG_FLAG
	std::cout << "*** Executing:";
	for(int i = 0; i < argc - 1; i++)
		std::cout << " " << argv_cmd[i];
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
		int retval = 0, local_errno = 0;
		if(from_path)
			retval = execvp(*argv_cmd, argv_cmd);
		else
			retval = execv(*argv_cmd, argv_cmd);

		local_errno = errno;
		if(retval < 0)
			std::cerr << "*** ERROR: execv" << (from_path ? "p" : "") << "() failed. Errno: " << local_errno << std::endl;

		exit(local_errno);
	}
	else
	{
		while(wait(&status) != pid);
#ifdef DEBUG_FLAG
		std::cout << "*** Child process (pid: " << pid << ", ppid: " << getppid() << ") terminated.";
#endif
		if(WIFEXITED(status) && WEXITSTATUS(status) != 0)
		{
#ifdef DEBUG_FLAG
			std::cout << " Return value: " << strerror(WEXITSTATUS(status)) << " (" << WEXITSTATUS(status) << ")";
#endif
			if(WEXITSTATUS(status) == ENOENT)
			{
				QMessageBox qm(QMessageBox::Critical, "Errore", "Errore: qemu-nbd non trovato.\nAssicurarsi che sia installato e di avere i privilegi per eseguirlo", QMessageBox::Close);
				qm.setPalette(MainWindow::getPalette());
				for(int i = 0; i < qm.buttons().size(); i++)
				{
					switch(qm.standardButton(qm.buttons()[i]))
					{
						case QDialogButtonBox::Close: qm.buttons()[i]->setText("Chiudi"); break;
					}
				}
				qm.exec();
				exit(ENOENT);
			}
		}
#ifdef DEBUG_FLAG
		std::cout << std::endl;
#endif
	}

	if(WIFEXITED(status))
		return WEXITSTATUS(status);
	else
		return status;
}

bool OSBridge::checkNbdModule()
{
	char *argv[3];
	argv[0] = (char *)malloc(sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 3)); strcpy(argv[0], NBDTOOL_CMD_NAME);
	argv[1] = (char *)malloc(sizeof(char) * (strlen("check") + 1)); strcpy(argv[1], "check");
	argv[2] = NULL;
	
	int retval = execute_cmd(3, argv, true);
#ifdef TRY_FROM_LOCAL_WD
	if(retval != 0)
	{
		argv[0] = (char *)realloc(argv[0], sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 3)); strcpy(argv[0], "./" NBDTOOL_CMD_NAME);
		retval = execute_cmd(5, argv);
	}
#endif
	return retval == 0;
}

bool OSBridge::cleanEnvironment(std::string tmp_dir)
{
	std::vector<std::string> tmp_content;

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(tmp_dir.c_str())) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
			if(ent->d_type == DT_DIR && strstr(ent->d_name, ".") == NULL)
				tmp_content.push_back(std::string(tmp_dir).append("/").append(ent->d_name));
		closedir(dir);

		for(int i = 0; i < tmp_content.size(); i++)
			if(tmp_content.at(i).find("-u"))
				umountVpartition(tmp_content.at(i));

		for(int i = 0; i < tmp_content.size(); i++)
			if(!tmp_content.at(i).find("-u"))
				umountVpartition(tmp_content.at(i));
	}
	
	if ((dir = opendir("/dev/")) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			if(strstr(ent->d_name, "nbd") != NULL && strstr(ent->d_name, "p") == NULL)
				umountVHD(std::string("/dev/").append(ent->d_name));
		}
		closedir(dir);
	}
	return true;
}

bool OSBridge::loadNbdModule(int devices, int partitions)
{
	std::stringstream devices_ss;    devices_ss << devices;
	std::stringstream partitions_ss; partitions_ss << partitions;

	char *argv[5];
	argv[0] = (char *)malloc(sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 1)); strcpy(argv[0], NBDTOOL_CMD_NAME);
	argv[1] = (char *)malloc(sizeof(char) * (strlen("load") + 1)); strcpy(argv[1], "load");
	argv[2] = (char *)malloc(sizeof(char) * (devices_ss.str().length() + 1)); strcpy(argv[2], devices_ss.str().c_str());
	argv[3] = (char *)malloc(sizeof(char) * (partitions_ss.str().length() + 1)); strcpy(argv[3], partitions_ss.str().c_str());
	argv[4] = NULL;

	int retval = execute_cmd(5, argv, true);
#ifdef TRY_FROM_LOCAL_WD
	if(retval != 0)
	{
		argv[0] = (char *)realloc(argv[0], sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 3)); strcpy(argv[0], "./" NBDTOOL_CMD_NAME);
		retval = execute_cmd(5, argv);
	}
#endif
	return retval == 0;
}

bool OSBridge::unloadNbdModule()
{
	char *argv[3];
	argv[0] = (char *)malloc(sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 1)); strcpy(argv[0], NBDTOOL_CMD_NAME);
	argv[1] = (char *)malloc(sizeof(char) * (strlen("unload") + 1)); strcpy(argv[1], "unload");
	argv[2] = NULL;
	
	int retval = execute_cmd(3, argv, true);
#ifdef TRY_FROM_LOCAL_WD
	if(retval != 0)
	{
		argv[0] = (char *)realloc(argv[0], sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 3)); strcpy(argv[0], "./" NBDTOOL_CMD_NAME);
		retval = execute_cmd(3, argv);
	}
#endif
	return retval == 0;
}

bool OSBridge::mountVHD(std::string source, std::string target)
{
	char *argv[5];
	argv[0] = (char *)malloc(sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 1)); strcpy(argv[0], NBDTOOL_CMD_NAME);
	argv[1] = (char *)malloc(sizeof(char) * (strlen("mountVHD") + 1)); strcpy(argv[1], "mountVHD");
	argv[2] = (char *)malloc(sizeof(char) * (source.length() + 1)); strcpy(argv[2], source.c_str());
	argv[3] = (char *)malloc(sizeof(char) * (target.length() + 1)); strcpy(argv[3], target.c_str());
	argv[4] = NULL;
	
	int retval = execute_cmd(5, argv, true);
#ifdef TRY_FROM_LOCAL_WD
	if(retval != 0)
	{
		argv[0] = (char *)realloc(argv[0], sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 3)); strcpy(argv[0], "./" NBDTOOL_CMD_NAME);
		retval = execute_cmd(5, argv);
	}
#endif
	return retval == 0;
}

bool OSBridge::umountVHD(std::string target)
{
	char *argv[4];
	argv[0] = (char *)malloc(sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 1)); strcpy(argv[0], NBDTOOL_CMD_NAME);
	argv[1] = (char *)malloc(sizeof(char) * (strlen("umountVHD") + 1)); strcpy(argv[1], "umountVHD");
	argv[2] = (char *)malloc(sizeof(char) * (target.length() + 1)); strcpy(argv[2], target.c_str());
	argv[3] = NULL;
	
	int retval = execute_cmd(4, argv, true);
#ifdef TRY_FROM_LOCAL_WD
	if(retval != 0)
	{
		argv[0] = (char *)realloc(argv[0], sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 3)); strcpy(argv[0], "./" NBDTOOL_CMD_NAME);
		retval = execute_cmd(4, argv);
	}
#endif
	return retval == 0;
}

bool OSBridge::mountVpartition(std::string source, std::string target, std::string user_target, bool readonly)
{
	struct stat s;

	if(stat(target.c_str(), &s) < 0 && errno == ENOENT)
		mkdir(target.c_str(), 0777);

	if(stat(user_target.c_str(), &s) < 0 && errno == ENOENT)
		mkdir(user_target.c_str(), 0777);

	char *argv[7];
	argv[0] = (char *)malloc(sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 1)); strcpy(argv[0], NBDTOOL_CMD_NAME);
	argv[1] = (char *)malloc(sizeof(char) * (strlen("mount") + 1)); strcpy(argv[1], "mount");
	argv[2] = (char *)malloc(sizeof(char) * (source.length() + 1)); strcpy(argv[2], source.c_str());
	argv[3] = (char *)malloc(sizeof(char) * (target.length() + 1)); strcpy(argv[3], target.c_str());
	argv[4] = (char *)malloc(sizeof(char) * (user_target.length() + 1)); strcpy(argv[4], user_target.c_str());
	argv[5] = (char *)malloc(sizeof(char) * (3));

	if(readonly)
		strcpy(argv[5], "ro");
	else
		strcpy(argv[5], "rw");

	argv[6] = NULL;
	
	int retval = execute_cmd(7, argv, true);
#ifdef TRY_FROM_LOCAL_WD
	if(retval != 0)
	{
		argv[0] = (char *)realloc(argv[0], sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 3)); strcpy(argv[0], "./" NBDTOOL_CMD_NAME);
		retval = execute_cmd(6, argv);
	}
#endif
	return retval == 0;
}

bool OSBridge::umountVpartition(std::string target)
{
	char *argv[4];
	argv[0] = (char *)malloc(sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 1)); strcpy(argv[0], NBDTOOL_CMD_NAME);
	argv[1] = (char *)malloc(sizeof(char) * (strlen("umount") + 1)); strcpy(argv[1], "umount");
	argv[2] = (char *)malloc(sizeof(char) * (target.length() + 1)); strcpy(argv[2], target.c_str());
	argv[3] = NULL;
	
	int retval = execute_cmd(4, argv, true);
#ifdef TRY_FROM_LOCAL_WD
	if(retval != 0)
	{
		argv[0] = (char *)realloc(argv[0], sizeof(char) * (strlen(NBDTOOL_CMD_NAME) + 3)); strcpy(argv[0], "./" NBDTOOL_CMD_NAME);
		retval = execute_cmd(4, argv);
	}
#endif
	if(retval == 0)
	{
		struct stat s;
		if(stat(target.c_str(), &s) >= 0 && ((s.st_mode & S_IFMT) == S_IFDIR))
			return rmdir(target.c_str()) == 0;
	}
	
	return retval == 0;
}

