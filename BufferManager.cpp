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
		/* 初始化自由队列 */
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

	// 查找缓存是否存在
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
	{
		if (bp->b_blkno != blkno)
			continue;
		bp->b_flags |= Buf::B_DONE;
		break;
		//return bp;
	}
	//std::cout << "Miss Buffer" << std::endl;

	// 不存在
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
	/* bp取下 */	
	bp->b_back->b_forw = bp->b_forw;
	bp->b_forw->b_back = bp->b_back;
	/* 挂至链尾 */
	bp->b_back = this->bFreeList.b_back;
	this->bFreeList.b_back->b_forw = bp;
	bp->b_forw = &this->bFreeList;
	this->bFreeList.b_back = bp;

	bp->b_blkno = blkno;
	return bp;
}

void BufferManager::Brelse(Buf* bp)
{
	/* 注意以下操作并没有清除B_DELWRI、B_WRITE、B_READ、B_DONE标志
	 * B_DELWRI表示虽然将该控制块释放到自由队列里面，但是有可能还没有些到磁盘上。
	 * B_DONE则是指该缓存的内容正确地反映了存储在或应存储在磁盘上的信息
	 */
	bp->b_flags &= ~(Buf::B_WANTED | Buf::B_BUSY | Buf::B_ASYNC);
	return;
}

Buf* BufferManager::Bread(int blkno)
{
	Buf* bp;
	/* 根据设备号，字符块号申请缓存 */
	bp = this->GetBlk(blkno);
	/* 如果在设备队列中找到所需缓存，即B_DONE已设置，就不需进行I/O操作 */
	if (bp->b_flags & Buf::B_DONE)
	{
		return bp;
	}
	/* 没有找到相应缓存，构成I/O读请求块 */
	bp->b_flags |= Buf::B_READ;
	bp->b_wcount = BufferManager::BUFFER_SIZE;

	// 直接放入缓存
	memcpy(bp->b_addr, &p[BufferManager::BUFFER_SIZE * blkno], BufferManager::BUFFER_SIZE);

	return bp;
}

void BufferManager::Bwrite(Buf *bp)
{
	unsigned int flags;

	flags = bp->b_flags;
	bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_ERROR | Buf::B_DELWRI);
	bp->b_wcount = BufferManager::BUFFER_SIZE;		/* 512字节 */

	if ((flags & Buf::B_ASYNC) == 0)
	{
		/* 同步写，需要等待I/O操作结束（纯文件系统 哪来的IO） */
		memcpy(&this->p[BufferManager::BUFFER_SIZE*bp->b_blkno], bp->b_addr, BufferManager::BUFFER_SIZE);
		this->Brelse(bp);
	}
	else if ((flags & Buf::B_DELWRI) == 0)
	{
		/*纯文件系统，不考虑出错情况，直接写 */
		memcpy(&this->p[BufferManager::BUFFER_SIZE*bp->b_blkno], bp->b_addr, BufferManager::BUFFER_SIZE);
		this->Brelse(bp);
	}
	return;
}

void BufferManager::Bdwrite(Buf *bp)
{
	/* 置上B_DONE允许其它进程使用该磁盘块内容 */
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	this->Brelse(bp);
	return;
}

void BufferManager::Bawrite(Buf *bp)
{
	/* 标记为异步写 */
	bp->b_flags |= Buf::B_ASYNC;
	this->Bwrite(bp);
	return;
}

void BufferManager::ClrBuf(Buf *bp)
{
	int* pInt = (int *)bp->b_addr;

	/* 将缓冲区中数据清零 */
	for (unsigned int i = 0; i < BufferManager::BUFFER_SIZE / sizeof(int); i++)
	{
		pInt[i] = 0;
	}
	return;
}

void BufferManager::Bflush()
{
	Buf* bp;
	/* 单进程直接搞就行了，不用考虑中断什么的情况 */
	for (bp = this->bFreeList.b_forw; bp != &(this->bFreeList); bp = bp->b_forw)
	{
		//	cout << "该缓存块指向哪一个磁盘" << bp->b_blkno << endl;
			/* 找出自由队列中所有延迟写的块 */
		if ((bp->b_flags & Buf::B_DELWRI))
		{
			//	cout << "找到延迟写的块" << endl;
				//注：将它放在队列的尾部
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

