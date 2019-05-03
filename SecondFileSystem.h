#pragma once

//#ifndef SECOND_FILE_SYSTEM_H
//#define SECOND_FILE_SYSTEM_H

#include "Kernel.h"

class SecondFileSystem
{
private:
	const int DEFAULT_MODE = 040755;

public:
	SecondFileSystem();
	~SecondFileSystem();

public:
	/* �ļ����� */
	int fopen(char* path, int mode);
	int fclose(int fd);
	/* �ļ���д */
	int fread(int fd, char* buffer, int length);
	int fwrite(int fd, char* buffer, int length);
	/* �ļ����� */
	int fseek(int fd, int pos, int ptrname);
	int fcreate(char* filename, int mode);
	int fdelete(char* filename);
	/* Ŀ¼���� */
	int ls();
	int mkdir(char* dirname);
	int cd(char* dirname);
	int bkdir();
};

//#endif
