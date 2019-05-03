#pragma once

//#ifndef FILE_SYSTEM_H
//#define FILE_SYSTEM_H

#include "INode.h"
#include "BufferManager.h"

/*
 * �ļ�ϵͳ�洢��Դ�����(Super Block)�Ķ��塣
 */
class SuperBlock
{
	/* Functions */
public:
	/* Constructors */
	SuperBlock();
	/* Destructors */
	~SuperBlock();

	/* Members */
public:
	int		s_isize;		/* ���Inode��ռ�õ��̿��� */
	int		s_fsize;		/* �̿����� */

	int		s_nfree;		/* ֱ�ӹ���Ŀ����̿����� */
	int		s_free[100];	/* ֱ�ӹ���Ŀ����̿������� */

	int		s_ninode;		/* ֱ�ӹ���Ŀ������Inode���� */
	int		s_inode[100];	/* ֱ�ӹ���Ŀ������Inode������ */

	int		s_fmod;			/* �ڴ���super block�������޸ı�־����ζ����Ҫ��������Ӧ��Super Block */
	int		s_ronly;		/* ���ļ�ϵͳֻ�ܶ��� */
	int		s_time;			/* ���һ�θ���ʱ�� */
	int		padding[49];	/* ���ʹSuperBlock���С����1024�ֽڣ�ռ��2������ */
};


/*
 * �ļ�ϵͳ��(FileSystem)�����ļ��洢�豸��
 * �ĸ���洢��Դ�����̿顢���INode�ķ��䡢
 * �ͷš�
 */
class FileSystem
{
public:
	/* static consts */
	static const int SUPER_BLOCK_SECTOR_NUMBER = 0;	/* ����SuperBlockλ�ڴ����ϵ������ţ�ռ��1, 2���������� */

	static const int ROOTINO = 0;			/* �ļ�ϵͳ��Ŀ¼���Inode��� */

	static const int INODE_NUMBER_PER_SECTOR = 8;		/* ���INode���󳤶�Ϊ64�ֽڣ�ÿ�����̿���Դ��512/64 = 8�����Inode */
	static const int INODE_ZONE_START_SECTOR = 2;		/* ���Inode��λ�ڴ����ϵ���ʼ������ */
	static const int INODE_ZONE_SIZE = /* 1024 - 202 */ 1000 - 2;		/* ���������Inode��ռ�ݵ������� */

	static const int DATA_ZONE_START_SECTOR = /* 1024 */ 1000;		/* ����������ʼ������ */
	static const int DATA_ZONE_END_SECTOR = /* 18000 - 1 */ 10000 - 1;	/* �������Ľ��������� */
	static const int DATA_ZONE_SIZE = 10000 - DATA_ZONE_START_SECTOR;	/* ������ռ�ݵ��������� */

	/* Functions */
public:
	/* Constructors */
	FileSystem();
	/* Destructors */
	~FileSystem();

	/*
	 * @comment ��ʼ����Ա����
	 */
	void Initialize();

	/*
	* @comment ϵͳ��ʼ��ʱ����SuperBlock
	*/
	void LoadSuperBlock();

	/*
	 * @comment �����ļ��洢�豸���豸��dev��ȡ
	 * ���ļ�ϵͳ��SuperBlock
	 */
	SuperBlock* GetFS();
	/*
	 * @comment ��SuperBlock������ڴ渱�����µ�
	 * �洢�豸��SuperBlock��ȥ
	 */
	void Update();

	/*
	 * @comment  �ڴ洢�豸dev�Ϸ���һ������
	 * ���INode��һ�����ڴ����µ��ļ���
	 */
	Inode* IAlloc();
	/*
	 * @comment  �ͷŴ洢�豸dev�ϱ��Ϊnumber
	 * �����INode��һ������ɾ���ļ���
	 */
	void IFree(int number);

	/*
	 * @comment �ڴ洢�豸dev�Ϸ�����д��̿�
	 */
	Buf* Alloc();
	/*
	 * @comment �ͷŴ洢�豸dev�ϱ��Ϊblkno�Ĵ��̿�
	 */
	void Free(int blkno);

private:
	/*
	 * @comment ����豸dev�ϱ��blkno�Ĵ��̿��Ƿ�����
	 * �����̿���
	 */
	bool BadBlock(SuperBlock* spb, int blkno);

	/* Members */
private:
	BufferManager* m_BufferManager;		/* FileSystem����Ҫ�������ģ��(BufferManager)�ṩ�Ľӿ� */
};

//#endif
