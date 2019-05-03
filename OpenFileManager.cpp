#include <iostream>

#include "OpenFileManager.h"
#include "Kernel.h"

// ȫ��superblockʵ��
SuperBlock g_spb;

/*==============================class OpenFileTable===================================*/
/* ϵͳȫ�ִ��ļ������ʵ���Ķ��� */
OpenFileTable g_OpenFileTable;

OpenFileTable::OpenFileTable()
{
	//nothing to do here
}

OpenFileTable::~OpenFileTable()
{
	//nothing to do here
}

/*���ã����̴��ļ������������ҵĿ�����  ֮ �±�  д�� u_ar0[EAX]*/
File* OpenFileTable::FAlloc()
{
	int fd;
	User& u = Kernel::Instance().GetUser();

	/* �ڽ��̴��ļ����������л�ȡһ�������� */
	fd = u.u_ofiles.AllocFreeSlot();

	if (fd < 0)	/* ���Ѱ�ҿ�����ʧ�� */
	{
		return NULL;
	}

	for (int i = 0; i < OpenFileTable::NFILE; i++)
	{
		/* f_count==0��ʾ������� */
		if (this->m_File[i].f_count == 0)
		{
			/* ������������File�ṹ�Ĺ�����ϵ */
			u.u_ofiles.SetF(fd, &this->m_File[i]);
			/* ���Ӷ�file�ṹ�����ü��� */
			this->m_File[i].f_count++;
			/* ����ļ�����дλ�� */
			this->m_File[i].f_offset = 0;
			return (&this->m_File[i]);
		}
	}

	std::cout << "No Free File Struct" << std::endl;
	u.u_error = User::my_ENFILE;
	return NULL;
}

void OpenFileTable::CloseF(File *pFile)
{
	if (pFile->f_count <= 1)
	{
		/*
		 * �����ǰ���������һ�����ø��ļ��Ľ��̣�
		 * ��������豸���ַ��豸�ļ�������Ӧ�Ĺرպ����������ڸ������
		 */
		//pFile->f_inode->CloseI(pFile->f_flag & File::FWRITE);
		g_InodeTable.IPut(pFile->f_inode);
	}

	/* ���õ�ǰFile�Ľ�������1 */
	pFile->f_count--;
}

/*==============================class InodeTable===================================*/
/*  �����ڴ�Inode���ʵ�� */
InodeTable g_InodeTable;

InodeTable::InodeTable()
{
	//nothing to do here
}

InodeTable::~InodeTable()
{
	//nothing to do here
}

void InodeTable::Initialize()
{
	/* ��ȡ��g_FileSystem������ */
	this->m_FileSystem = &Kernel::Instance().GetFileSystem();
}

Inode* InodeTable::IGet(int inumber)
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	while (true)
	{
		/* ���ָ���豸dev�б��Ϊinumber�����Inode�Ƿ����ڴ濽�� */
		int index = this->IsLoaded(inumber); 
		//std::cout << "inumber = " << inumber << ", index = " << index << std::endl;

		if (index >= 0)	/* �ҵ��ڴ濽�� */
		{
			pInode = &(this->m_Inode[index]);
			/* ������ڴ�Inode�����������ο��赥���̣��������������� */
			/*if (pInode->i_flag & Inode::ILOCK)
			{
				// ����IWANT��־��Ȼ��˯��
				pInode->i_flag |= Inode::IWANT;

				u.u_procp->Sleep((unsigned long)&pInode, ProcessManager::PINOD);

				// �ص�whileѭ������Ҫ������������Ϊ���ڴ�Inode�����Ѿ�ʧЧ
				continue;
			} */

			/* ������ڴ�Inode�����������ļ�ϵͳ�����Ҹ�Inode��Ӧ��Mountװ��飬���β��������ļ�ϵͳ */
			/*if (pInode->i_flag & Inode::IMOUNT)
			{
				Mount* pMount = this->m_FileSystem->GetMount(pInode);
				if (NULL == pMount)
				{
					// û���ҵ� 
					Utility::Panic("No Mount Tab...");
				}
				else
				{
					// ��������Ϊ���ļ�ϵͳ�豸�š���Ŀ¼Inode��� 
					dev = pMount->m_dev;
					inumber = FileSystem::ROOTINO;
					// �ص�whileѭ��������dev��inumberֵ��������
					continue;
				}
			}*/

			/*
			 * ����ִ�е������ʾ���ڴ�Inode���ٻ������ҵ���Ӧ�ڴ�Inode��
			 * ���������ü���������ILOCK��־������֮
			 */
			pInode->i_count++;

			//pInode->i_flag |= Inode::ILOCK;
			return pInode;
		}
		else	/* û��Inode���ڴ濽���������һ�������ڴ�Inode */
		{
			pInode = this->GetFreeInode();
			/* ���ڴ�Inode���������������Inodeʧ�� */
			if (NULL == pInode)
			{
				std::cout << "Inode Table Overflow" << std::endl;
				u.u_error = User::my_ENFILE;
				return NULL;
			}
			else	/* �������Inode�ɹ��������Inode�����·�����ڴ�Inode */
			{
				/* �����µ��豸�š����Inode��ţ��������ü������������ڵ����� */
				//pInode->i_dev = dev;
				pInode->i_number = inumber;
				//pInode->i_flag = Inode::ILOCK;
				pInode->i_count++;
				pInode->i_lastr = -1;

				BufferManager& bm = Kernel::Instance().GetBufferManager();
				/* �������Inode���뻺���� */
				Buf* pBuf = bm.Bread(FileSystem::INODE_ZONE_START_SECTOR + inumber / FileSystem::INODE_NUMBER_PER_SECTOR);

				/* �������I/O���� */
				if (pBuf->b_flags & Buf::B_ERROR)
				{
					/* �ͷŻ��� */
					bm.Brelse(pBuf);
					/* �ͷ�ռ�ݵ��ڴ�Inode */
					this->IPut(pInode);
					return NULL;
				}

				/* ���������е����Inode��Ϣ�������·�����ڴ�Inode�� */
				pInode->ICopy(pBuf, inumber);
				/* �ͷŻ��� */
				bm.Brelse(pBuf);
				return pInode;
			}
		}
	}
	return NULL;	/* GCC likes it! */
}

/* close�ļ�ʱ�����Iput
 *      ��Ҫ���Ĳ������ڴ�i�ڵ���� i_count--����Ϊ0���ͷ��ڴ� i�ڵ㡢���иĶ�д�ش���
 * �����ļ�;��������Ŀ¼�ļ������������󶼻�Iput���ڴ�i�ڵ㡣·�����ĵ�����2��·������һ���Ǹ�
 *   Ŀ¼�ļ�������������д������ļ�������ɾ��һ�������ļ���������������д���ɾ����Ŀ¼����ô
 *   	���뽫���Ŀ¼�ļ�����Ӧ���ڴ� i�ڵ�д�ش��̡�
 *   	���Ŀ¼�ļ������Ƿ��������ģ����Ǳ��뽫����i�ڵ�д�ش��̡�
 * */
void InodeTable::IPut(Inode *pNode)
{
	/* ��ǰ����Ϊ���ø��ڴ�Inode��Ψһ���̣���׼���ͷŸ��ڴ�Inode */
	if (pNode->i_count == 1)
	{
		/* ���ļ��Ѿ�û��Ŀ¼·��ָ���� */
		if (pNode->i_nlink <= 0)
		{
			/* �ͷŸ��ļ�ռ�ݵ������̿� */
			pNode->ITrunc();
			pNode->i_mode = 0;
			/* �ͷŶ�Ӧ�����Inode */
			this->m_FileSystem->IFree(pNode->i_number);
		}

		/* �������Inode��Ϣ */
		pNode->IUpdate();

		/* ����ڴ�Inode�����б�־λ */
		pNode->i_flag = 0;
		/* �����ڴ�inode���еı�־֮һ����һ����i_count == 0 */
		pNode->i_number = -1;
	}

	/* �����ڴ�Inode�����ü��������ѵȴ����� */
	pNode->i_count--;
}

void InodeTable::UpdateInodeTable()
{
	for (int i = 0; i < InodeTable::NINODE; i++)
	{
		/*
		 * ���Inode����û�б�����������ǰδ����������ʹ�ã�����ͬ�������Inode��
		 * ����count������0��count == 0��ζ�Ÿ��ڴ�Inodeδ���κδ��ļ����ã�����ͬ����
		 */
		if (this->m_Inode[i].i_count != 0)
		{
			/* ���ڴ�Inode������ͬ�������Inode */
			this->m_Inode[i].i_flag |= Inode::ILOCK;
			this->m_Inode[i].IUpdate();
		}
	}
}

int InodeTable::IsLoaded(int inumber)
{
	/* Ѱ��ָ�����Inode���ڴ濽�� */
	for (int i = 0; i < InodeTable::NINODE; i++)
	{
		if (this->m_Inode[i].i_number == inumber && this->m_Inode[i].i_count != 0)
		{
			return i;
		}
	}
	return -1;
}

Inode* InodeTable::GetFreeInode()
{
	for (int i = 0; i < InodeTable::NINODE; i++)
	{
		/* ������ڴ�Inode���ü���Ϊ�㣬���Inode��ʾ���� */
		if (this->m_Inode[i].i_count == 0)
		{
			return &(this->m_Inode[i]);
		}
	}
	return NULL;	/* Ѱ��ʧ�� */
}
