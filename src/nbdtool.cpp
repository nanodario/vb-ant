#include <iostream>
#include <stdio.h>

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <libkmod.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <string.h>

std::string handle_error(int err)
{
	std::string str;
	switch (err)
	{
		case ENOEXEC:
			str = "Invalid module format";
		case ENOENT:
			str = "Unknown symbol in module";
		case ESRCH:
			str = "Module has wrong symbol version";
		case EINVAL:
			str = "Invalid parameters";
		default:
			str = strerror(-err);
	}
	return str;
}

int check_module_inuse(kmod_module *mod)
{
	struct kmod_list *holders;
	int state;

	state = kmod_module_get_initstate(mod);

	if (state == KMOD_MODULE_BUILTIN)
	{
		std::cerr << "Module " << kmod_module_get_name(mod) << " is builtin." << std::endl;
		return -ENOENT;
	}
	else if (state < 0)
	{
		std::cerr << "Module " << kmod_module_get_name(mod) << " is not currently loaded" << std::endl;
		return -ENOENT;
	}

	holders = kmod_module_get_holders(mod);
	if (holders != NULL)
	{
		struct kmod_list *itr;

		std::cerr << "Module " << kmod_module_get_name(mod) << " is in use by: ";
		
		kmod_list_foreach(itr, holders)
		{
			struct kmod_module *hm = kmod_module_get_module(itr);
			std::cerr << kmod_module_get_name(hm);
			kmod_module_unref(hm);
		}
		std::cerr << std::endl;

		kmod_module_unref_list(holders);
		return -EBUSY;
	}

	if (kmod_module_get_refcnt(mod) != 0)
	{
		std::cerr << "Module " << kmod_module_get_name(mod) << " is in use" << std::endl;
		return -EBUSY;
	}

	return 0;
}

int insert_module(int devices = 16, int partitions = 16)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	const char *null_config = NULL;
	int err;

	struct utsname u;

	if (uname(&u) < 0)
	{
		std::cerr << "*** " << getpid() << " *** ERROR: uname() failed" << std::endl;
		err = -1;
	}

	std::string dirname = "/lib/modules/";
	dirname.append(u.release);

	ctx = kmod_new(dirname.c_str(), &null_config);
	if(ctx == NULL)
		exit(EXIT_FAILURE);

	err = kmod_module_new_from_name(ctx, "nbd", &mod);
	if(err != 0)
	{
		std::cerr << "*** " << getpid() << " *** ERROR: creating module from path: " << handle_error(err) << std::endl;
		return err;
	}

	int state = kmod_module_get_initstate(mod);
	if (state == KMOD_MODULE_BUILTIN)
	{
		std::cerr << "Module " << kmod_module_get_name(mod) << " is builtin." << std::endl;
		return -ENOENT;
	}
	else if (state > 0)
	{
		std::cerr << "Module " << kmod_module_get_name(mod) << " is currently loaded" << std::endl;
		return -EEXIST;
	}

	std::stringstream devices_ss;
	devices_ss << "nbds_max=" << devices << " max_part=" << partitions;

	std::cout << "Max devices: " << devices_ss.str() << std::endl;
	err = kmod_module_insert_module(mod, 0, devices_ss.str().c_str());
	if(err == -EEXIST)
		return err;
	else if(err != 0)
	{
		std::cerr << "*** " << getpid() << " *** ERROR: inserting module: " << handle_error(err) << std::endl;
		return err;
	}

	kmod_unref(ctx);
	return 0;
}

int remove_module()
{
	struct kmod_ctx *ctx;
	const char *null_config = NULL;
	int flags = KMOD_REMOVE_NOWAIT;
	int err;

	ctx = kmod_new(NULL, &null_config);
	if (!ctx)
	{
		std::cerr << "*** " << getpid() << " *** ERROR: kmod_new() failed!" << std::endl;
		return EXIT_FAILURE;
	}

	struct kmod_module *mod;
	err = kmod_module_new_from_name(ctx, "nbd", &mod);

	if (err < 0)
	{
		std::cerr << "*** " << getpid() << " *** ERROR: could not use module nbd: " << handle_error(err) << std::endl;
		kmod_unref(ctx);
		return err;
	}

	if (check_module_inuse(mod) < 0)
	{
		kmod_module_unref(mod);
		return -EBUSY;
	}

	err = kmod_module_remove_module(mod, flags);
	if (err < 0)
	{
		std::cerr << "*** " << getpid() << " *** ERROR: could not remove module nbd: " << handle_error(err) << std::endl;
		return err;
	}

	kmod_module_unref(mod);
	kmod_unref(ctx);

	return 0;
}

int check_module()
{
	struct kmod_ctx *ctx;
	const char *null_config = NULL;
	struct kmod_list *list, *itr;
	int err;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
	{
		std::cerr << "*** " << getpid() << " *** ERROR: kmod_new() failed!" << std::endl;
		return -ENOMEM;
	}

	err = kmod_module_new_from_loaded(ctx, &list);
	if (err < 0)
	{
		std::cerr << "*** " << getpid() << " *** ERROR: could not get list of modules: " << strerror(-err) << std::endl;;
		kmod_unref(ctx);
		return -EPERM;
	}

	bool module_loaded = false;
	kmod_list_foreach(itr, list) 
	{
		struct kmod_module *mod = kmod_module_get_module(itr);
		const char *name = kmod_module_get_name(mod);
		if(!strcmp(name, "nbd"))
			module_loaded = true;
		
		kmod_module_unref(mod);
	}
	kmod_module_unref_list(list);
	kmod_unref(ctx);

	if(module_loaded)
		return 0;
	else
		return -ENOENT;
}

int execute_cmd(int argc, char **argv)
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
		std::cerr << "*** " << getpid() << " *** ERROR: fork() failed" << std::endl;
		return 1;
	}
	else if(pid == 0)
	{
		if(execvp(*argv, argv) < 0)
		{
			std::cerr << "*** " << getpid() << " *** ERROR: exec() failed" << std::endl;
			return 2;
		}
	}
	else
	{
		while(wait(&status) != pid);
		std::cout << "Return value: " << status << std::endl;
	}

	return status;
}

int do_mountVHD(std::string source, std::string target)
{
	std::string cmd = "qemu-nbd";
	std::string opt = "-c";

	char *argv_new[5];

	argv_new[0] = (char *)malloc(sizeof(char) * (cmd.length() + 1)); strcpy(argv_new[0], cmd.c_str());
	argv_new[1] = (char *)malloc(sizeof(char) * (opt.length() + 1)); strcpy(argv_new[1], opt.c_str());
	argv_new[2] = (char *)malloc(sizeof(char) * (target.length() + 1)); strcpy(argv_new[2], target.c_str());
	argv_new[3] = (char *)malloc(sizeof(char) * (source.length() + 1)); strcpy(argv_new[3], source.c_str());
	argv_new[4] = NULL;

	int retval = setuid(0);
	if(retval != 0)
	{
		perror("setuid(): ");
		return retval;
	}
	
	retval = setgid(0);
	if(retval != 0)
	{
		perror("setgid(): ");
		return retval;
	}

	return execute_cmd(5, argv_new);
}

int do_umountVHD(std::string target)
{
	std::string cmd = "qemu-nbd";
	std::string opt = "-d";

	char *argv_new[4];

	argv_new[0] = (char *)malloc(sizeof(char) * (cmd.length() + 1)); strcpy(argv_new[0], cmd.c_str());
	argv_new[1] = (char *)malloc(sizeof(char) * (opt.length() + 1)); strcpy(argv_new[1], opt.c_str());
	argv_new[2] = (char *)malloc(sizeof(char) * (target.length() + 1)); strcpy(argv_new[2], target.c_str());
	argv_new[3] = NULL;

	int retval = setuid(0);
	if(retval != 0)
	{
		perror("setuid(): ");
		return retval;
	}
	
	retval = setgid(0);
	if(retval != 0)
	{
		perror("setgid(): ");
		return retval;
	}

	return execute_cmd(4, argv_new);
}

int do_mount(std::string source, std::string mountpoint, bool readonly = false)
{
	std::string cmd = "mount";

	int retval = setuid(0);

	if(retval != 0)
	{
		perror("setuid(): ");
		return retval;
	}

	retval = setgid(0);
	if(retval != 0)
	{
		perror("setgid(): ");
		return retval;
	}
	
	if(readonly)
	{
		char *argv_new[6];
		std::string opt1 = "-o";
		std::string opt2 = "ro";

		argv_new[0] = (char *)malloc(sizeof(char) * (cmd.length() + 1)); strcpy(argv_new[0], cmd.c_str());
		argv_new[1] = (char *)malloc(sizeof(char) * (source.length() + 1)); strcpy(argv_new[1], source.c_str());
		argv_new[2] = (char *)malloc(sizeof(char) * (mountpoint.length() + 1)); strcpy(argv_new[2], mountpoint.c_str());
		argv_new[3] = (char *)malloc(sizeof(char) * (opt1.length() + 1)); strcpy(argv_new[3], opt1.c_str());
		argv_new[4] = (char *)malloc(sizeof(char) * (opt2.length() + 1)); strcpy(argv_new[4], opt2.c_str());
		argv_new[5] = NULL;

		retval = execute_cmd(6, argv_new);
	}
	else
	{
		char *argv_new[4];

		argv_new[0] = (char *)malloc(sizeof(char) * (cmd.length() + 1)); strcpy(argv_new[0], cmd.c_str());
		argv_new[1] = (char *)malloc(sizeof(char) * (source.length() + 1)); strcpy(argv_new[1], source.c_str());
		argv_new[2] = (char *)malloc(sizeof(char) * (mountpoint.length() + 1)); strcpy(argv_new[2], mountpoint.c_str());
		argv_new[3] = NULL;

		retval = execute_cmd(4, argv_new);
	}

	return retval;
}

int do_bindmount(std::string mountpoint, std::string user_mountpoint, uid_t uid, uid_t gid)
{
	std::string cmd = "bindfs";
	std::stringstream opt1; opt1 << "-u";
	std::stringstream opt2; opt2 << uid;
	std::stringstream opt3; opt3 << "-g";
	std::stringstream opt4; opt4 << gid;
	
	char *argv_new[8];
	
	argv_new[0] = (char *)malloc(sizeof(char) * (cmd.length() + 1)); strcpy(argv_new[0], cmd.c_str());
	argv_new[1] = (char *)malloc(sizeof(char) * (opt1.str().length() + 1)); strcpy(argv_new[1], opt1.str().c_str());
	argv_new[2] = (char *)malloc(sizeof(char) * (opt2.str().length() + 1)); strcpy(argv_new[2], opt2.str().c_str());
	argv_new[3] = (char *)malloc(sizeof(char) * (opt3.str().length() + 1)); strcpy(argv_new[3], opt3.str().c_str());
	argv_new[4] = (char *)malloc(sizeof(char) * (opt4.str().length() + 1)); strcpy(argv_new[4], opt4.str().c_str());
	argv_new[5] = (char *)malloc(sizeof(char) * (mountpoint.length() + 1)); strcpy(argv_new[5], mountpoint.c_str());
	argv_new[6] = (char *)malloc(sizeof(char) * (user_mountpoint.length() + 1)); strcpy(argv_new[6], user_mountpoint.c_str());
	argv_new[7] = NULL;
	
	return execute_cmd(8, argv_new);
}

int do_umount(std::string mountpoint)
{
	std::string cmd = "umount";

	char *argv_new[3];

	argv_new[0] = (char *)malloc(sizeof(char) * (cmd.length() + 1)); strcpy(argv_new[0], cmd.c_str());
	argv_new[1] = (char *)malloc(sizeof(char) * (mountpoint.length() + 1)); strcpy(argv_new[1], mountpoint.c_str());
	argv_new[2] = NULL;

	int retval = setuid(0);
	if(retval != 0)
	{
		perror("setuid(): ");
		return retval;
	}

	retval = setgid(0);
	if(retval != 0)
	{
		perror("setgid(): ");
		return retval;
	}

	return execute_cmd(3, argv_new);
}

int main(int argc, char **argv)
{
	/*
	 * nbdtool:
	 * 	load		[max_devices [max_partitions]]
	 * 	unload
	 * 	mountVHD	VHD dev_mountpoint
	 * 	umountVHD	dev_mountpoint
	 * 	mount		Vpartition mountpoint user_mountpoint [ro|rw]
	 * 	umount		mountpoint
	 */

#ifndef DEBUG_FLAG
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
#endif

	uid_t original_uid = getuid();
	uid_t original_gid = getgid();

	int retval = -1;

	if(argc > 4)
	{
		if(!strcmp(argv[1], "mount"))
		{
			bool readonly = false;
			if(argc > 5)
				readonly = strcmp(argv[5], "ro") == 0;
			
			if(!check_module())
			{
				if((retval = do_mount(argv[2], argv[3], readonly)) == 0)
					retval = do_bindmount(argv[3], argv[4], original_uid, original_gid);
			}
			else
				std::cerr << "*** " << getpid() << " *** ERROR: nbd module is not loaded, cannot mount Vpartition" << std::endl;
		}
	}
	
	if(argc > 3)
	{
		if(!strcmp(argv[1], "mountVHD"))
		{
			if(!check_module())
				retval = do_mountVHD(argv[2], argv[3]);
			else
				std::cerr << "*** " << getpid() << " *** ERROR: nbd module is not loaded, cannot mount VHD" << std::endl;
		}
	}

	if(argc > 2)
	{
		if(!strcmp(argv[1], "umountVHD"))
		{
			if(!check_module())
				retval = do_umountVHD(argv[2]);
			else
				std::cerr << "*** " << getpid() << " *** ERROR: nbd module is not loaded, cannot unmount VHD" << std::endl;
		}
		
		if(!strcmp(argv[1], "umount"))
		{
			if(!check_module())
				retval = do_umount(argv[2]);
			else
				std::cerr << "*** " << getpid() << " *** ERROR: nbd module is not loaded, cannot unmount Vpartition" << std::endl;
		}
	}

	if(argc > 1)
	{
		if(!strcmp(argv[1], "load"))
		{
			if(setuid(0) == 0 && setgid(0) == 0)
			{
				if(argc > 3 && atoi(argv[2]) && atoi(argv[3]))
					retval = insert_module(atoi(argv[2]), atoi(argv[3]));
				else if(argc > 2 && atoi(argv[2]))
					retval = insert_module(atoi(argv[2]));
				else
					retval = insert_module();
				std::cout << "nbd module is" << (check_module() == 0 ? " " : " not ") << "loaded" << std::endl;
			}
			else
			{
				std::cerr << "*** " << getpid() << " *** ERROR: can't set uid/gid" << std::endl;
				retval = -1;
			}
		}
		else if(!strcmp(argv[1], "unload"))
		{
			if(setuid(0) == 0 && setgid(0) == 0)
			{
				retval = remove_module();
				std::cout << "nbd module is" << (check_module() == 0 ? " " : " not ") << "loaded" << std::endl;
			}
			else
			{
				std::cerr << "*** " << getpid() << " *** ERROR: can't set uid/gid" << std::endl;
				retval = -1;
			}
		}
		else if(!strcmp(argv[1], "check"))
		{
			retval = check_module();
			std::cout << "nbd module is" << (check_module() == 0 ? " " : " not ") << "loaded" << std::endl;
		}
	}

	if(argc == 1)
	{
		retval = check_module();
		std::cout << "nbd module is" << (retval == 0 ? " " : " not ") << "loaded" << std::endl;	
	}

	setuid(original_uid);
	setgid(original_gid);

#ifdef DEBUG_FLAG
	if(retval != 0)
		std::cerr << "*** " << getpid() << " *** ERROR: " << retval << std::endl;
#endif

	return retval;
}
