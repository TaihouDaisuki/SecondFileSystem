#include <cstring>

#include "Kernel.h"
#include "BufferManager.h"

BufferManager::BufferManager()
{
	//nothing to do here
}

BufferManager::~BufferManager()
{
	//nothing to do here
}

void BufferManager::Initialize(char *start)
{
	this->p = start;
	int i;
	Buf* bp;

	this->bFreeList.b_forw = this->bFreeList.b_back = &(this->bFreeList);

	for (i = 0; i < NBUF; i++)
	{
		bp = &(this->m_Buf[i]);
		bp->b_addr = this->Buffer[i];
		/* ��ʼ�����ɶ��� */
		bp->b_back = &(this->bFreeList);
		bp->b_forw = this->bFreeList.b_forw;
		this->bFreeList.b_forw->b_back = bp;
		this->bFreeList.b_forw = bp;
		bp->b_flags = Buf::B_BUSY;
		Brelse(bp);
	}

	return;
}

Buf* BufferManager::GetBlk(int blkno)
{
	Buf* dp = &this->bFreeList;
	Buf* bp;

	// ���һ����Ƿ����
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
	{
		if (bp->b_blkno != blkno)
			continue;
		bp->b_flags |= Buf::B_DONE;
		break;
		//return bp;
	}
	//std::cout << "Miss Buffer" << std::endl;

	// ������
	if(bp == dp)
	{
		bp = this->bFreeList.b_forw;
		if (bp->b_flags & Buf::B_DELWRI)
		{
			//std::cout << "Write!!" << std::endl;
			this->Bwrite(bp);
		}
		bp->b_flags = Buf::B_BUSY;
	}

	/* LRU */
	/* bpȡ�� */	
	bp->b_back->b_forw = bp->b_forw;
	bp->b_forw->b_back = bp->b_back;
	/* ������β */
	bp->b_back = this->bFreeList.b_back;
	this->bFreeList.b_back->b_forw = bp;
	bp->b_forw = &this->bFreeList;
	this->bFreeList.b_back = bp;

	bp->b_blkno = blkno;
	return bp;
}

void BufferManager::Brelse(Buf* bp)
{
	/* ע�����²�����û�����B_DELWRI��B_WRITE��B_READ��B_DONE��־
	 * B_DELWRI��ʾ��Ȼ���ÿ��ƿ��ͷŵ����ɶ������棬�����п��ܻ�û��Щ�������ϡ�
	 * B_DONE����ָ�û����������ȷ�ط�ӳ�˴洢�ڻ�Ӧ�洢�ڴ����ϵ���Ϣ
	 */
	bp->b_flags &= ~(Buf::B_WANTED | Buf::B_BUSY | Buf::B_ASYNC);
	return;
}

Buf* BufferManager::Bread(int blkno)
{
	Buf* bp;
	/* �����豸�ţ��ַ�������뻺�� */
	bp = this->GetBlk(blkno);
	/* ������豸�������ҵ����軺�棬��B_DONE�����ã��Ͳ������I/O���� */
	if (bp->b_flags & Buf::B_DONE)
	{
		return bp;
	}
	/* û���ҵ���Ӧ���棬����I/O������� */
	bp->b_flags |= Buf::B_READ;
	bp->b_wcount = BufferManager::BUFFER_SIZE;

	// ֱ�ӷ��뻺��
	memcpy(bp->b_addr, &p[BufferManager::BUFFER_SIZE * blkno], BufferManager::BUFFER_SIZE);

	return bp;
}

void BufferManager::Bwrite(Buf *bp)
{
	unsigned int flags;

	flags = bp->b_flags;
	bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_ERROR | Buf::B_DELWRI);
	bp->b_wcount = BufferManager::BUFFER_SIZE;		/* 512�ֽ� */

	if ((flags & Buf::B_ASYNC) == 0)
	{
		/* ͬ��д����Ҫ�ȴ�I/O�������������ļ�ϵͳ ������IO�� */
		memcpy(&this->p[BufferManager::BUFFER_SIZE*bp->b_blkno], bp->b_addr, BufferManager::BUFFER_SIZE);
		this->Brelse(bp);
	}
	else if ((flags & Buf::B_DELWRI) == 0)
	{
		/*���ļ�ϵͳ�������ǳ��������ֱ��д */
		memcpy(&this->p[BufferManager::BUFFER_SIZE*bp->b_blkno], bp->b_addr, BufferManager::BUFFER_SIZE);
		this->Brelse(bp);
	}
	return;
}

void BufferManager::Bdwrite(Buf *bp)
{
	/* ����B_DONE������������ʹ�øô��̿����� */
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	this->Brelse(bp);
	return;
}

void BufferManager::Bawrite(Buf *bp)
{
	/* ���Ϊ�첽д */
	bp->b_flags |= Buf::B_ASYNC;
	this->Bwrite(bp);
	return;
}

void BufferManager::ClrBuf(Buf *bp)
{
	int* pInt = (int *)bp->b_addr;

	/* ������������������ */
	for (unsigned int i = 0; i < BufferManager::BUFFER_SIZE / sizeof(int); i++)
	{
		pInt[i] = 0;
	}
	return;
}

void BufferManager::Bflush()
{
	Buf* bp;
	/* ������ֱ�Ӹ�����ˣ����ÿ����ж�ʲô����� */
	for (bp = this->bFreeList.b_forw; bp != &(this->bFreeList); bp = bp->b_forw)
	{
		//	cout << "�û����ָ����һ������" << bp->b_blkno << endl;
			/* �ҳ����ɶ����������ӳ�д�Ŀ� */
		if ((bp->b_flags & Buf::B_DELWRI))
		{
			//	cout << "�ҵ��ӳ�д�Ŀ�" << endl;
				//ע���������ڶ��е�β��
			bp->b_back->b_forw = bp->b_forw;
			bp->b_forw->b_back = bp->b_back;

			bp->b_back = this->bFreeList.b_back->b_forw;
			this->bFreeList.b_back->b_forw = bp;
			bp->b_forw = &this->bFreeList;
			this->bFreeList.b_back = bp;

			this->Bwrite(bp);
		}
	}
	return;
}

Buf* BufferManager::InCore(int blkno)
{
	Buf* dp = &this->bFreeList;
	Buf* bp;

	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
	{
		if (bp->b_blkno == blkno)
			return bp;
	}
	return NULL;
}

Buf& BufferManager::GetBFreeList()
{
	return this->bFreeList;
}

