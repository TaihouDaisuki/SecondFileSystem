#pragma once

//#ifndef KERNEL_H
//#define KERNEL_H

#include "User.h"
#include "BufferManager.h"
#include "FileSystem.h"
#include "FileManager.h"

/*
 * Kernel�����ڷ�װ�����ں���ص�ȫ����ʵ������
 * ����PageManager, ProcessManager�ȡ�
 *
 * Kernel�����ڴ���Ϊ����ģʽ����֤�ں��з�װ���ں�
 * ģ��Ķ���ֻ��һ��������
 */
class Kernel
{
public:
	Kernel();
	~Kernel();
	static Kernel& Instance();
	void Initialize(char* p);		/* �ú�����ɳ�ʼ���ں˴󲿷����ݽṹ�ĳ�ʼ�� */

	BufferManager& GetBufferManager();
	FileSystem& GetFileSystem();
	FileManager& GetFileManager();
	User& GetUser();		/* ��ȡ��ǰ���̵�User�ṹ */

private:
	void InitBuffer(char* p);
	void InitFileSystem();

private:
	static Kernel instance;		/* Kernel������ʵ�� */

	BufferManager* m_BufferManager;
	FileSystem* m_FileSystem;
	FileManager* m_FileManager;
	User* m_User;
};

//#endif
