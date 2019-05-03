#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cstring>
#include <string>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "SecondFileSystem.h"

using namespace std;

SecondFileSystem sfs;

const int TaihouDaisuki = -2; // Mark for test

char *imgbuffer;
int imgsize; /* >0: wait to write; <0: wait to save */

void init_spb(SuperBlock &spb)
{
	spb.s_isize = FileSystem::INODE_ZONE_SIZE;
	spb.s_fsize = FileSystem::DATA_ZONE_END_SECTOR + 1;

	//第一组99块 其他都是一百块一组 剩下的被超级快直接管理
	spb.s_nfree = (FileSystem::DATA_ZONE_SIZE - 99) % 100;

	//超级快直接管理的空闲盘块的第一个盘块的盘块号
	int start_last_datablk = FileSystem::DATA_ZONE_START_SECTOR;
	while (1)
	{
		if ((start_last_datablk + 100 - 1) < FileSystem::DATA_ZONE_END_SECTOR)
			//剩下盘块是否还有100个
			start_last_datablk += 100;
		else
			break;
	}
		
	--start_last_datablk;
	for (int i = 0; i < spb.s_nfree; ++i)
		spb.s_free[i] = start_last_datablk + i;

	spb.s_ninode = 100;
	for (int i = 0; i < spb.s_ninode; ++i)
		spb.s_inode[i] = i;

	spb.s_fmod = 0;
	spb.s_ronly = 0;
}
void init_datablock(char *data)
{
	struct 
	{
		int nfree;//本组空闲的个数
		int free[100];//本组空闲的索引表
	}tmp_table;

	int last_datablk_num = FileSystem::DATA_ZONE_SIZE;
		//未加入索引的盘块的数量

	for (int i = 0; ; ++i)
	{
		if (last_datablk_num >= 100)
			tmp_table.nfree = 100;
		else
			tmp_table.nfree = last_datablk_num;
		last_datablk_num -= tmp_table.nfree;

		for (int j = 0; j < tmp_table.nfree; ++j)
			tmp_table.free[j] = (!i && !j) ? 0 : (100 * i + j + FileSystem::DATA_ZONE_START_SECTOR - 1);
		memcpy(&data[99 * 512 + i * 100 * 512], (void*)&tmp_table, sizeof(tmp_table));
		if (last_datablk_num == 0)
			break;
	}
}
int init_img(int fd)
{
	SuperBlock spb;
	init_spb(spb);
	DiskInode *di = new DiskInode[FileSystem::INODE_ZONE_SIZE * FileSystem::INODE_NUMBER_PER_SECTOR];

	//设置rootDiskInode的初始值
	di[0].d_mode = Inode::IFDIR;
	di[0].d_mode |= Inode::IEXEC;

	char *datablock = new char[FileSystem::DATA_ZONE_SIZE * 512];
	memset(datablock, 0, FileSystem::DATA_ZONE_SIZE * 512);
	init_datablock(datablock);

	write(fd, &spb, sizeof(SuperBlock));
	write(fd, di, FileSystem::INODE_ZONE_SIZE * FileSystem::INODE_NUMBER_PER_SECTOR * sizeof(DiskInode));
	write(fd, datablock, FileSystem::DATA_ZONE_SIZE * 512);

	cout << "==========格式化磁盘完毕=============" << endl;
	//	exit(1);
}

void initSFS()
{
	FileManager& filemanager = Kernel::Instance().GetFileManager();
	filemanager.rootDirInode = g_InodeTable.IGet(FileSystem::ROOTINO);
	filemanager.rootDirInode->i_flag &= (~Inode::ILOCK);

	Kernel::Instance().GetFileSystem().LoadSuperBlock();
	User& u = Kernel::Instance().GetUser();
	u.u_cdir = g_InodeTable.IGet(FileSystem::ROOTINO);

	strcpy(u.u_curdir, "/");

	cout << "==========系统初始化完毕==========" << endl << endl;
}
void quitSFS(char *addr, int length)
{
	BufferManager &bm = Kernel::Instance().GetBufferManager();
	bm.Bflush();
	msync(addr, length, MS_SYNC);
	InodeTable *it = Kernel::Instance().GetFileManager().m_InodeTable;
	it->UpdateInodeTable();

	cout << "==========The system quit successfully==========" << endl;
	cout << "================Thanks for using================" << endl;
}

void input_flush()
{
	while (getchar() != '\n');
}

void output_help()
{
	cout << "******************************************" << endl;
	cout << "* The language supported are as followed *" << endl;
	cout << "*========================================*" << endl;
	cout << "*             Operation Part             *" << endl;
	cout << "*----------------------------------------*" << endl;
	cout << "* -fopen [filename] [mode]               *" << endl;
	cout << "* -fclose [fd]                           *" << endl;
	cout << "* -fread [fd] [length]                   *" << endl;
	cout << "* -fwrite [fd] [source] [length]         *" << endl;
	cout << "* -fseek [fd] [position] [ptrname]       *" << endl;
	cout << "* -fcreate [filename] [mode]             *" << endl;
	cout << "* -fdelete [filename]                    *" << endl;
	cout << "* -ls                                    *" << endl;
	cout << "* -mkdir [dirname]                       *" << endl;
	cout << "* -cd [dirname]                          *" << endl;
	cout << "*       (P.S. -cd ../ means <backdir>)   *" << endl;
	cout << "* -quit                                  *" << endl;
	cout << "*========================================*" << endl;
	cout << "*                Test Part               *" << endl;
	cout << "*----------------------------------------*" << endl;
	cout << "* -fread -2 [fd] [length]                *" << endl;
	cout << "*       (Read to buffer)                 *" << endl;
	cout << "* -fwrite -2 [fd]                        *" << endl;
	cout << "*        (Write from buffer)             *" << endl;
	cout << "* -loadimg                               *" << endl;
	cout << "*       (Load img to buffer from outside)*" << endl;
	cout << "* -saveimg                               *" << endl;
	cout << "*       (Save img from buffer to outside)*" << endl;
	cout << "* -check                                 *" << endl;
	cout << "*       (check the buffer state)         *" << endl;
	cout << "******************************************" << endl;

	input_flush();
}

/* Operation Part */
void fopen_work()
{
	string filename;
	int mode;
	cin >> filename >> mode;

	char *filename_c = new char[filename.length() + 1];
	strcpy(filename_c, filename.c_str());

	int fd = sfs.fopen(filename_c, mode);
	if (fd < 0)
		cout << "< Error: Open file failed >" << endl;
	else
		cout << "< Success: The return file fd = " << fd << " >" <<endl;
	delete filename_c;

	input_flush();
}
void fclose_work()
{
	int fd;
	cin >> fd;
	sfs.fclose(fd);

	input_flush();
}
void fread_work()
{
	int fd, length;
	cin >> fd;
	
	if(fd == TaihouDaisuki)
	{
		cin >> fd >> length;
		if(imgsize > 0)
		{
			cout << "< Error: Buffer wait to be written >" << endl;
			input_flush();
			return;
		}
		else if(imgsize < 0)
		{
			cout << "< Warning: Img has been read >" << endl;
			input_flush();
			return;
		}

		imgbuffer = new char[length + 1];
		int res = sfs.fread(fd, imgbuffer, length);
		cout << "< Success: Fread returns = " << res << " >" << endl;
		if(res > 0)
			imgsize = -res;
		else
		{
			imgsize = 0;
			delete imgbuffer;
			imgbuffer = NULL;
		}
	}
	else
	{
		cin  >> length;
		char *buffer= new char[length + 1];
		memset(buffer, 0, length + 1);

		int res = sfs.fread(fd, buffer, length);
		cout << "< Success: Fread returns = " << res << " >" << endl;
		cout << "< Success: The data read are as followed >" << endl;
		cout << buffer << endl;
		delete buffer;
	}

	input_flush();
}
void fwrite_work()
{
	int fd, length;
	string buffer;
	cin >> fd;

	if(fd == TaihouDaisuki)
	{
		cin >> fd;
		if(imgsize == 0)
		{
			cout << "< Error: Img hasn't been loaded >" << endl;
			input_flush();
			return;
		}
		else if(imgsize < 0)
		{
			cout << "< Error: Buffer wait to be saved >" << endl;
			input_flush();
			return;
		}

		int res = sfs.fwrite(fd, imgbuffer, imgsize);
		cout << "< Success: Fwrite returns = " << res << " >" << endl;
		
		if(res > 0)
		{
			imgsize = 0;
			delete imgbuffer;
			imgbuffer = NULL;
		}
	}
	else
	{
		cin >> buffer >> length;
		char *buffer_c = new char[buffer.length() + 1];
		strcpy(buffer_c, buffer.c_str());

		int res = sfs.fwrite(fd, buffer_c, length);
		cout << "< Success: Fwrite returns = " << res << " >" << endl;
		delete buffer_c;
	}

	input_flush();
}
void fseek_work()
{
	int fd, position, ptrname;
	cin >> fd >> position >> ptrname;

	int res = sfs.fseek(fd, position, ptrname);
	cout << "< Success: Fseek returns = " << res << " >" << endl;

	input_flush();
}
void fcreate_work()
{
	string filename;
	int mode;
	cin >> filename >> mode;

	char *filename_c = new char[filename.length() + 1];
	strcpy(filename_c, filename.c_str());

	int fd = sfs.fcreate(filename_c, mode);
	if (fd < 0)
	{
		cout << "< Error: Failed to create file >" << endl;
	}
	else
	{
		cout << "< Success: The fd of created file = " << fd << " >" << endl;
	}
	delete filename_c;

	input_flush();
}
void fdelete_work()
{
	string filename;
	cin >> filename;

	char *filename_c = new char[filename.length() + 1];
	strcpy(filename_c, filename.c_str());
	sfs.fdelete(filename_c);

	delete filename_c;
	input_flush();
}
void ls_work()
{
	cout << "< The ls report get the returns as followed >" << endl;
	cout << "---------------------------------------------" << endl;
	sfs.ls();
	cout << "---------------------------------------------" << endl;

	input_flush();
}
void mkdir_work()
{
	string dirname;
	cin >> dirname;

	char *dirname_c = new char[dirname.length() + 1];
	strcpy(dirname_c, dirname.c_str());
	sfs.mkdir(dirname_c);
	delete dirname_c;

	input_flush();
}
void cd_work()
{
	string dirname;
	cin >> dirname;

	char *dirname_c = new char[dirname.length() + 1];
	strcpy(dirname_c, dirname.c_str());

	if (!strcmp(dirname_c, "../"))
		sfs.bkdir();
	else
		sfs.cd(dirname_c);
	delete dirname_c;

	input_flush();
}

/* Test Part */
void load_work()
{
	input_flush();

	if(imgsize > 0)
	{
		cout << "< Warning: Img has been loaded >" << endl;
		return;
	}
	else if(imgsize < 0)
	{
		cout << "< Error: Img wait to be saved > " << endl;
		return;
	}
	

	int imgfd = open("img_in.png", O_RDONLY);
	if(imgfd == -1)
	{
		cout << "< Error: No img file for test >" << endl;
		return;
	}
	struct stat imgst; 
	int res = fstat(imgfd, &imgst);
	if (res == -1) 
	{
		cout << "< Error: Failed to get img file information >" << endl;
		close(imgfd);
		return;
	}

	imgsize = imgst.st_size;
	imgbuffer = new char[imgsize];
	read(imgfd, imgbuffer, imgsize);
	cout << "< Attention: Img file loaded successfully >" << endl;

	close(imgfd);
}
void save_work()
{
	input_flush();

	if(imgsize > 0)
	{
		cout << "< Error: Img has been loaded >" << endl;
		return;
	}
	else if(imgsize == 0)
	{
		cout << "< Error: Empty buffer > " << endl;
		return;
	}

	int imgfd = open("img_out.png", O_WRONLY | O_CREAT);
	if(imgfd == -1)
	{
		cout << "< Error: Fail to create file >" << endl;
		return;
	}

	write(imgfd, imgbuffer, -imgsize);
	cout << "< Attention: Img file saved successfully >" << endl;

	delete imgbuffer;
	imgbuffer = NULL;
	imgsize = 0;
	close(imgfd);
}
void check_work()
{
	input_flush();

	if(imgsize == 0)
	{
		cout << "< Attention: Buffer is empty >" << endl;
	}
	else if(imgsize > 0)
	{
		cout << "< Attention: Buffer has been loaded, size = " << imgsize << " >" << endl;
	}
	else /* img size < 0 */
	{
		cout << " < Attention: Buffer wait to be saved, size = " << imgsize << " >" << endl;
	}
}

int main()
{
	/* Load img file AND  Initialize the file system */
	int fd = open("myDisk.disk", O_RDWR);
	if (fd == -1) 
	{
		fd = open("myDisk.disk", O_RDWR | O_CREAT, 0666);
		if (fd == -1) 
		{
			cout << "Open file error" << endl;
			exit(-1);
		}
		init_img(fd);
	}

	struct stat st; 
	int res = fstat(fd, &st);
	if (res == -1) 
	{
		cout << "Failed to get file information" << endl;
		close(fd);
		exit(-1);
	}

	int fsize = st.st_size; 	//把文件映射成虚拟内存地址
	void* addr = mmap(NULL, fsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	Kernel::Instance().Initialize((char*)addr);
	initSFS();
	imgbuffer = NULL;
	imgsize = 0;

	/*************************************************************************/

	/* File system part */
	cout << "-----------------------------------------------------------------" << endl;
	cout << "|            The file system has already loaded                 |" << endl;
	cout << "|       Use -help to check the language it supported            |" << endl;
	cout << "-----------------------------------------------------------------" << endl;

	char op;
	string command;
	while (1)
	{
		cout << "SecondFileSystem@TaihouDaisuki> ";
		op = getchar();
		if (op == '\n')
			continue;

		if (op != '-')
		{
			input_flush();
			cout << "< Error: Unsustained Language >" << endl;
			continue;
		}

		cin >> command;
		if(!strcmp(command.c_str(), "help"))
		{
			output_help();
			continue;
		}

		if (!strcmp(command.c_str(), "fopen"))
		{
			fopen_work();
			continue;
		}

		if (!strcmp(command.c_str(), "fclose"))
		{
			fclose_work();
			continue;
		}

		if (!strcmp(command.c_str(), "fread"))
		{
			fread_work();
			continue;
		}

		if (!strcmp(command.c_str(), "fwrite"))
		{
			fwrite_work();
			continue;
		}

		if (!strcmp(command.c_str(), "fseek"))
		{
			fseek_work();
			continue;
		}

		if (!strcmp(command.c_str(), "fcreate"))
		{
			fcreate_work();
			continue;
		}

		if (!strcmp(command.c_str(), "fdelete"))
		{
			fdelete_work();
			continue;
		}

		if(!strcmp(command.c_str(), "ls"))
		{
			ls_work();
			continue;
		}

		if (!strcmp(command.c_str(), "mkdir"))
		{
			mkdir_work();
			continue;
		}

		if (!strcmp(command.c_str(), "cd"))
		{
			cd_work();
			continue;
		}

		if (!strcmp(command.c_str(), "quit"))
		{
			quitSFS((char*)addr, fsize);
			break;
		}

		if (!strcmp(command.c_str(), "loadimg"))
		{
			load_work();
			continue;
		}

		if (!strcmp(command.c_str(), "saveimg"))
		{
			save_work();
			continue;
		}

		if (!strcmp(command.c_str(), "check"))
		{
			check_work();
			continue;
		}

		input_flush();
		cout << "< Error: Unsustained Language >" << endl;
	}

	close(fd);
	if(imgbuffer != NULL)
		delete imgbuffer;

	return 0;
}
