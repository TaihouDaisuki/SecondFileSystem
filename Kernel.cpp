#include <iostream>

#include "Kernel.h"
#include "FileSystem.h"
#include "FileManager.h"

Kernel Kernel::instance;

/*
 * 高速缓存管理全局manager
 */
BufferManager g_BufferManager;

/*
 * 文件系统相关全局manager
 */
FileSystem g_FileSystem;
FileManager g_FileManager;

/*
 * 用户User结构
 */
User g_User;

Kernel::Kernel()
{
}

Kernel::~Kernel()
{
}

Kernel& Kernel::Instance()
{
	return Kernel::instance;
}


void Kernel::InitBuffer(char* p)
{
	this->m_BufferManager = &g_BufferManager;

	std::cout << "Initialize Buffer...";
	this->GetBufferManager().Initialize(p);
	std::cout << "OK." << std::endl;
}

void Kernel::InitFileSystem()
{
	this->m_FileSystem = &g_FileSystem;
	this->m_FileManager = &g_FileManager;
	this->m_User = &g_User;

	std::cout << "Initialize File System...";
	this->GetFileSystem().Initialize();
	std::cout << "OK." << std::endl;

	std::cout << "Initialize File Manager...";
	this->GetFileManager().Initialize();
	std::cout << "OK." << std::endl;
}

void Kernel::Initialize(char* p)
{
	InitBuffer(p);
	InitFileSystem();
}

BufferManager& Kernel::GetBufferManager()
{
	return *(this->m_BufferManager);
}

FileSystem& Kernel::GetFileSystem()
{
	return *(this->m_FileSystem);
}

FileManager& Kernel::GetFileManager()
{
	return *(this->m_FileManager);
}

User& Kernel::GetUser()
{
	return *(this->m_User);
}
