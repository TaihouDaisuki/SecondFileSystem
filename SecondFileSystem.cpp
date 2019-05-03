#include <iostream>
#include <cstring>

#include "SecondFileSystem.h"

SecondFileSystem::SecondFileSystem()
{
	// nothing to do
}
SecondFileSystem::~SecondFileSystem()
{
	// nothing to do
}

int SecondFileSystem::fopen(char* path, int mode)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_ar0 = 0;
	u.u_dirp = path;
	u.u_arg[1] = mode;

	FileManager &filemanager = Kernel::Instance().GetFileManager();
	filemanager.Open();

	return u.u_ar0;
}
int SecondFileSystem::fclose(int fd)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_ar0 = 0;
	u.u_arg[0] = fd;

	FileManager &filemanager = Kernel::Instance().GetFileManager();
	//u.u_arg[0] = fd;
	filemanager.Close();

	return 1;
}

int SecondFileSystem::fread(int fd, char* buffer, int length)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_ar0 = 0;
	u.u_arg[0] = fd;
	u.u_arg[1] = int(buffer);
	u.u_arg[2] = length;
	
	FileManager &filemanager = Kernel::Instance().GetFileManager();
	filemanager.Read();

	return u.u_ar0;
}
int SecondFileSystem::fwrite(int fd, char* buffer, int length)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_ar0 = 0;
	u.u_arg[0] = fd;
	u.u_arg[1] = int(buffer);
	u.u_arg[2] = length;

	FileManager &filemanager = Kernel::Instance().GetFileManager();
	filemanager.Write();

	return u.u_ar0;
}

int SecondFileSystem::fseek(int fd, int pos, int ptrname)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_ar0 = 0;
	u.u_arg[0] = fd;
	u.u_arg[1] = pos;
	u.u_arg[2] = ptrname;

	FileManager &filemanager = Kernel::Instance().GetFileManager();
	filemanager.Seek();

	return u.u_ar0;
}
int SecondFileSystem::fcreate(char* filename, int mode)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_ar0 = 0;
	u.u_dirp = filename;
	u.u_arg[1] = Inode::IRWXU;

	FileManager &filemanager = Kernel::Instance().GetFileManager();
	filemanager.Creat();

	return u.u_ar0;
}
int SecondFileSystem::fdelete(char* filename)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_ar0 = 0;
	u.u_dirp = filename;

	FileManager &filemanager = Kernel::Instance().GetFileManager();
	filemanager.UnLink();

	return 1;
}

int SecondFileSystem::ls()
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;

	int fd = fopen(u.u_curdir, File::FREAD);
	char filename[32] = "";
	while (1)
	{
		if (fread(fd, filename, 32) == 0)
			break;
		else
		{
			DirectoryEntry *cur = (DirectoryEntry *)filename;
			if (cur->m_ino == 0)
				continue;

			std::cout << "<< " << cur->m_name << " >>" << std::endl;

			memset(filename, 0, 32);
		}
	}
	return 1;
}
int SecondFileSystem::mkdir(char *dirname)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_dirp = dirname;
	u.u_arg[1] = DEFAULT_MODE;
	u.u_arg[2] = 0;

	FileManager &filemanager = Kernel::Instance().GetFileManager();
	filemanager.MkNod();

	return 1;
}
int SecondFileSystem::cd(char *dirname)
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;
	u.u_dirp = dirname;
	u.u_arg[0] = int(dirname);

	FileManager &filemanager = Kernel::Instance().GetFileManager();
	filemanager.ChDir();

	std::cout << "[" << u.u_curdir << "]" <<std::endl;

	return 1;
}
int SecondFileSystem::bkdir()
{
	User &u = Kernel::Instance().GetUser();
	u.u_error = User::my_NOERROR;

	char *predir = strrchr(u.u_curdir, '/');
	char newdir[128] = "";
	if (predir == u.u_curdir && u.u_curdir[1] == 0)
	{
		std::cout << "Warning: You are in the root dir" << std::endl;
		return 1;
	}
	if (predir == u.u_curdir)
	{
		std::cout << "Attention: You will return to the root dir" << std::endl;
		newdir[0] = '/';
	}
	else
	{
		int p = 0;
		for (char* pc = u.u_curdir; pc != predir; ++pc, ++p)
		{
			newdir[p] = u.u_curdir[p];
		}
	}

	std::cout << "[" << newdir << "]" <<std::endl;
	u.u_dirp = newdir;
	u.u_arg[0] = int(newdir);

	FileManager &filemanager = Kernel::Instance().GetFileManager();
	filemanager.ChDir();

	return 1;
}
