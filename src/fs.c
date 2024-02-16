#include "hdraw.h"
#include "fs.h"
#include "utility.h"
#include "list.h"

#ifdef DTFSER
  #include <malloc.h>
  #define Malloc malloc
  #define Free free
#else
  #include "memory.h"
  #define printf 
#endif

#define FS_MAGIC       "DTFS-v1.0"
#define ROOT_MAGIC     "ROOT"
#define HEADER_SCT_IDX 0                 //头信息扇区号
#define ROOT_SCT_IDX   1                 //根信息扇区号
#define FIXED_SCT_SIZE 2                 //固定扇区占用数
#define SCT_END_FLAG   ((uint)-1)
#define FE_BYTES       sizeof(FileEntry)
#define FD_BYTES       sizeof(FileDesc)
#define FE_ITEM_CNT    (SECT_SIZE/FE_BYTES)  //一个扇区内能保存几个文件描述单元
#define MAP_ITEM_CNT   (SECT_SIZE/sizeof(uint)) //一个扇区内的扇区分配单元个数

typedef struct
{
	byte forJmp[4];
	char magic[32];
	uint sctNum;    //扇区数
	uint mapSize;   //扇区分配表大小
	uint freeNum;   //空闲扇区数
	uint freeBegin; //空闲扇区起始位置
} FSHeader;

typedef struct
{
	char magic[32];
	uint sctBegin;  //起始扇区
	uint sctNum;    //总共占多少扇区
	uint lastBytes; //最后一个扇区内占多少大小
} FSRoot;

typedef struct
{
	char name[32];
	uint sctBegin;  //起始扇区
	uint sctNum;    //总共占多少扇区
	uint lastBytes; //最后一个扇区内占多少大小
	uint type;
	uint inSctIdx;  //文件描述单元在根目录第几个扇区
	uint inSctOff;  //文件描述单元在根目录第几个扇区中的偏移位置
	uint reserved[2];
} FileEntry;

typedef struct 
{
  ListNode head;   //链表头
	FileEntry fe;
	uint objIdx;    //当前的数据扇区编号
	uint offset;    //当前写入指针的偏移量
	uint changed;
	byte cache[SECT_SIZE]; //扇区缓冲区
} FileDesc;

typedef struct
{
	uint* pSct;   //扇区分配单元对应扇区的数据
	uint sctIdx;  //绝对扇区号
	uint sctOff;  //扇区分配表的第几个扇区
	uint idxOff;  //对应扇区的第几个扇区分配单元
} MapPos;

static List gFDList = {0}; //用于保存已打开的文件名

void FSModInit()
{
  HDRawModInit();

  List_Init(&gFDList);
}

static void* ReadSector(uint si)
{
	void* ret = NULL;

	if( si != SCT_END_FLAG )
	{
		ret = Malloc(SECT_SIZE);

		if( !(ret && HDRawRead(si, (byte*)ret)) )
		{
			Free(ret);
			ret = NULL;
		}
	}

	return ret;
}

static MapPos FindInMap(uint si)
{
	MapPos ret = {0};
	FSHeader* header = (si != SCT_END_FLAG) ? ReadSector(HEADER_SCT_IDX) : NULL;

	if( header )
	{
		uint offset = si - header->mapSize - FIXED_SCT_SIZE; //将扇区号转化为可分配扇区的起始的相对扇区号
		uint sctOff = offset / MAP_ITEM_CNT;   //计算扇区分配单元位置
		uint idxOff = offset % MAP_ITEM_CNT;
		uint* ps = ReadSector(sctOff + FIXED_SCT_SIZE); //读取扇区分配单元对应扇区

		if( ps )
		{
			ret.pSct = ps;
			ret.sctIdx = si;
			ret.sctOff = sctOff;
			ret.idxOff = idxOff;
		}
	}
	

	Free(header);

	return ret;
}

static uint AllocSector()
{
	uint ret = SCT_END_FLAG;
	FSHeader* header = ReadSector(HEADER_SCT_IDX);

	if( header && (header->freeBegin != SCT_END_FLAG) )
	{
		MapPos mp = FindInMap(header->freeBegin); //找到空闲节点的分配单元

		if( mp.pSct )
		{
			uint* pInt = AddrOff(mp.pSct, mp.idxOff); //对应的扇区分配单元
			uint next = *pInt;   //指向的下一个扇区
			uint flag = 1;

			ret = header->freeBegin;

			header->freeBegin = next + FIXED_SCT_SIZE + header->mapSize;  //转化成绝对扇区地址
			header->freeNum --;

			*pInt = SCT_END_FLAG; //标记当前扇区已被占用

            //将修改的扇区数据重新写会硬盘  
			flag = flag && HDRawWrite(HEADER_SCT_IDX, (byte *) header);
			flag = flag && HDRawWrite(mp.sctOff + FIXED_SCT_SIZE , (byte *)mp.pSct);

			if( !flag )
			{
				ret = SCT_END_FLAG;
			}
		}

		Free(mp.pSct);
	}

	Free(header);

	return ret;
}

static uint FreeSector(uint si)
{
	uint ret = 0;
	FSHeader* header = (si != SCT_END_FLAG) ? ReadSector(HEADER_SCT_IDX) : NULL;

	if( header )
	{
		MapPos mp = FindInMap(si); //找到要释放扇区的分配单元

		if( mp.pSct )
		{
			uint* pInt = AddrOff(mp.pSct, mp.idxOff); //找到对应的扇区分配单元

            *pInt = header->freeBegin - FIXED_SCT_SIZE - header->mapSize; //扇区分配单元内存放的是相对位置 转化成绝对扇区地址

			header->freeBegin = si; //将释放的扇区放在空闲扇区的首位
			header->freeNum ++;

			ret = HDRawWrite(HEADER_SCT_IDX, (byte*)header)
                  && HDRawWrite(mp.sctOff + FIXED_SCT_SIZE, (byte*)mp.pSct);
		}

		Free(mp.pSct);
	}

	Free(header);

	return ret;
	
}

void test_Alloc_free()
{
	printf("test()..\n");

	uint a[5] = {0};
	int i = 0;

	FSHeader* header = (FSHeader*)ReadSector(HEADER_SCT_IDX);

	printf("free sector = %d\n", header->freeNum);

	for(i=0; i<5; i++)
	{
		a[i] = AllocSector();
	}

	Free(header);
	
	header = (FSHeader*)ReadSector(HEADER_SCT_IDX);
	printf("free sector = %d\n", header->freeNum);

	for(i=0; i<5; i++)
	{
		printf("a[%d]= %d\n", i, a[i]);
		FreeSector(a[i]);
	}

	Free(header);

	header = (FSHeader*)ReadSector(HEADER_SCT_IDX);
	printf("free sector = %d\n", header->freeNum);

	
	Free(header);
}

static uint NextSector(uint si)
{
	uint ret = SCT_END_FLAG;
	FSHeader* header = (si != SCT_END_FLAG) ? ReadSector(HEADER_SCT_IDX) : NULL;

	if( header )
	{
		MapPos mp = FindInMap(si); //找到要释放扇区的分配单元

		if( mp.pSct )
		{
			uint* pInt = AddrOff(mp.pSct, mp.idxOff); //找到对应的扇区分配单元

			if(*pInt != SCT_END_FLAG)
			{
				ret = *pInt + header->mapSize + FIXED_SCT_SIZE; //返回值是绝对扇区地址
			}
 		}

		Free(mp.pSct);
	}

	Free(header);

	return ret;
    	
}

void test_next_sector()
{
	printf("test()..\n");

	uint n = 0;
	uint next = 0;

	FSHeader* header = (FSHeader*)ReadSector(HEADER_SCT_IDX);

	printf("free sector = %d\n", header->freeNum);

	next = header->freeBegin;

	while( next != SCT_END_FLAG )
	{
		n++;

		next = NextSector(next);
		
	}

	printf("n = %d\n", n);
	
	Free(header);
}

//找到最后一个扇区
static uint FindLast(uint sctBegin)
{
	uint ret = SCT_END_FLAG;
	uint next = sctBegin;

	while( next != SCT_END_FLAG )
	{
		ret = next;
		next = NextSector(next);
	}

	return ret;
}

//找到前一扇区
static uint FindPrev(uint sctBegin, uint si)
{
	uint ret = SCT_END_FLAG;
	uint next = sctBegin;

	while( (next != SCT_END_FLAG) && (next != si) )
	{
		ret = next;
		next = NextSector(next);
	}

	if( next == SCT_END_FLAG )
	{
		ret = SCT_END_FLAG;
	}

	return ret;
}

//找到以sctBegin扇区起始的第idx个扇区 返回值是绝对扇区地址
static uint FindIndex(uint sctBegin, uint idx)
{
	uint ret = sctBegin;
	uint i = 0;

  while( (i < idx) && ( ret != SCT_END_FLAG) )
  {
		ret = NextSector(ret);

		i++;
  }

	return ret;
}

//标记当前扇区是末尾扇区
static uint MarkSector(uint si)
{
	uint ret = (si == SCT_END_FLAG) ? 1 : 0;
	MapPos mp = FindInMap(si);

	if( mp.pSct )
	{
		uint *pInt = AddrOff(mp.pSct, mp.idxOff);

		*pInt = SCT_END_FLAG;

		ret = HDRawWrite(mp.sctOff + FIXED_SCT_SIZE, (byte *)mp.pSct);
	}

	Free(mp.pSct);

	return ret;
}

static void AddToLast(uint sctBegin, uint si)
{
	uint last = FindLast(sctBegin);

	if( last != SCT_END_FLAG )
	{
		//找到对应的扇区分配单元 
		MapPos lmp = FindInMap(last);
		MapPos smp = FindInMap(si);

		if( lmp.pSct &&  smp.pSct )
		{
			//判断要处理的是否在同一个扇区
			if( lmp.sctOff == smp.sctOff )
			{
				uint* pInt = AddrOff(lmp.pSct, lmp.idxOff);

				*pInt = lmp.sctOff * MAP_ITEM_CNT + smp.idxOff; //将last的扇区分配表上的值设为si在扇区分配表中的序号

				pInt = AddrOff(lmp.pSct, smp.idxOff);

				*pInt = SCT_END_FLAG;

				HDRawWrite(lmp.sctOff + FIXED_SCT_SIZE, (byte *)lmp.pSct);
			}
			else
			{
				uint* pInt = AddrOff(lmp.pSct, lmp.idxOff);

				*pInt = smp.sctOff * MAP_ITEM_CNT + smp.idxOff; //将last的扇区分配表上的值设为si在扇区分配表中的序号

				pInt = AddrOff(smp.pSct, smp.idxOff);

				*pInt = SCT_END_FLAG;

				HDRawWrite(lmp.sctOff + FIXED_SCT_SIZE, (byte *)lmp.pSct);
				HDRawWrite(smp.sctOff + FIXED_SCT_SIZE, (byte *)smp.pSct);				
			}


		}

		Free(lmp.pSct);
		Free(smp.pSct);
	}
}

static uint CheckStorage(FSRoot* fe)
{
	uint ret = 0;

  //判断扇区是否已经被写满了
	if( fe->lastBytes == SECT_SIZE )
	{
		uint si = AllocSector();  //分配一个新扇区

		if( si != SCT_END_FLAG )
		{
			if( fe->sctBegin == SCT_END_FLAG )
			{
				fe->sctBegin = si;
			}
			else
			{
				AddToLast(fe->sctBegin, si);
			}

			fe->sctNum ++;
			fe->lastBytes = 0;

			ret = 1;
		}
	}

	return ret;
}

static uint CreateFileEntry(const char* name, uint sctBegin, uint lastBytes)
{
	uint ret = 0;
	uint last = FindLast(sctBegin);
	FileEntry* feBase = NULL;

	if( (last != SCT_END_FLAG) && (feBase = (FileEntry*)ReadSector(last)) )
	{
		uint offset = lastBytes / FE_BYTES; //得到空闲的文件描述单元的位置
		FileEntry* fe = AddrOff(feBase, offset);

		StrCpy(fe->name, name, sizeof(fe->name) - 1); //拷贝文件名

		fe->type = 0;
		fe->sctBegin = SCT_END_FLAG;
		fe->sctNum = 0;
		fe->inSctIdx = last;
		fe->inSctOff = offset;
		fe->lastBytes = SECT_SIZE;

		ret = HDRawWrite(last, (byte *) feBase);		
	}
	
    Free(feBase);

	return ret;
}

static uint CreateInRoot(const char* name)
{
	FSRoot* root = (FSRoot*)ReadSector(ROOT_SCT_IDX);
	uint ret = 0;

    if( root )
    {
        CheckStorage(root);

		if( CreateFileEntry(name, root->sctBegin, root->lastBytes) )
		{
			root->lastBytes += FE_BYTES;

			ret = HDRawWrite(ROOT_SCT_IDX, (byte*)root);
		}
    }

	Free(root);

	return ret;
}

void test_create_root()
{
	uint r = CreateInRoot("test.txt");

	printf("create = %d\n", r);

	if( r )
	{
		FSRoot* root = (FSRoot*)ReadSector(ROOT_SCT_IDX);
		FileEntry* feBase = (FileEntry*)ReadSector(root->sctBegin);
		int i = 0;
		int n = 0;

		printf("sctNum = %d\n", root->sctNum);
		printf("lastBytes = %d\n", root->lastBytes);

		n = root->lastBytes / FE_BYTES;

		for(i=0; i<n; i++)
		{
			FileEntry* fe = AddrOff(feBase, i);
			printf("name = %s\n", fe->name);
		}

        Free(feBase);
		Free(root);
	}
}

static FileEntry* FindInSector(const char* name, FileEntry* febase, uint cnt)
{
	FileEntry* ret = NULL;
    uint i = 0;

  //遍历传入的文件描述符单元
	for(i=0; i<cnt; i++)
	{
		FileEntry* fe = AddrOff(febase, i);

    //比较名字
		if( StrCmp(fe->name, name, -1) )
		{
			ret = (FileEntry*)Malloc(FE_BYTES);

			if( ret )
			{
				*ret = *fe; //结构体赋值确实可以这样 但是属于浅拷贝
			}

			break;
		}
	}
   

	return ret;
}

static FileEntry* FindFileEntry(const char* name, uint sctBegin, uint sctNum, uint lastBytes)
{
	FileEntry* ret = NULL;
	uint next = sctBegin;
	uint i = 0;

  //先查找最后一个扇区前面的所有扇区
	for(i=0; i<(sctNum-1); i++)
	{
		FileEntry* febase = (FileEntry*)ReadSector(next);

		if( febase )
		{
			ret = FindInSector(name, febase, FE_ITEM_CNT);
		}

		Free(febase);

		if( !ret )
		{
			next = NextSector(next);
		}
		else
		{
			break;
		}
	}

  //查找完前面的所有扇区后 
  //如果还没有找到则继续查找最后一个扇区
	if( !ret )
	{
		uint cnt = lastBytes/FE_BYTES;
    FileEntry* febase = (FileEntry*)ReadSector(next);
	
    if( febase )
    {
			ret = FindInSector(name, febase, cnt);
    }

		Free(febase);
	}

	return ret;
}

static FileEntry* FindInRoot(const char* name)
{
	FSRoot* root = (FSRoot*)ReadSector(ROOT_SCT_IDX);  //读取根目录扇区信息
	FileEntry* ret = NULL;

	if( root && root->sctNum )
	{
		ret = FindFileEntry(name, root->sctBegin, root->sctNum, root->lastBytes);
	}

  Free(root);
	
	return ret;
}

void test_findInRoot()
{
	if( FindInRoot("test.txt") )
		printf("test.txt is found!\n");
}

uint FCreate(const char* fn)
{
	uint ret = FExisted(fn);

	if( ret == FS_NONEXISTED )
	{
		ret = CreateInRoot(fn) ? FS_SUCCEED : FS_FAILED;
	}

	return ret;
}

uint FExisted(const char* fn)
{
	uint ret = FS_FAILED;

	if( fn )
	{
		FileEntry* fe =FindInRoot(fn);

		ret = fe ? FS_EXISTED : FS_NONEXISTED;

		Free(fe);
	}

	return ret;
}

static uint IsOpened(const char* name)
{
	uint ret = 0;
	ListNode* pos = NULL;

	List_ForEach(&gFDList, pos)
	{
	  FileDesc* fd = (FileDesc*)pos;

	  if( StrCmp(fd->fe.name, name, -1) )
	  {
	     ret = 1;
	     break;
	  }
	
	}

	return ret;
}

static uint FreeFile(uint sctBegin)
{
	uint slider = sctBegin;
	uint ret = 0;

    //遍历并释放所有扇区
	while( slider != SCT_END_FLAG )
	{
		uint next = NextSector(slider);

		ret += FreeSector(slider);

		slider = next;
	}

	return ret;
}

static void MoveFileEntry(FileEntry* dst, FileEntry* src)
{
	uint inSctIdx = dst->inSctIdx;
	uint inSctOff = dst->inSctOff;

	*dst = *src;

	dst->inSctIdx = inSctIdx;
	dst->inSctOff = inSctOff;
}

//调整存储空间 主要是调整扇区
static uint AdjustStorage(FSRoot* fe)
{
	uint ret = 0;

	if( !fe->lastBytes )
	{
		uint last = FindLast(fe->sctBegin);
		uint prev = FindPrev(fe->sctBegin, last);

        //释放最后一个扇区 并标记最后一个扇区的前一个扇区为末尾
		if( FreeSector(last) && MarkSector(prev) )
		{
			fe->sctNum--;
			fe->lastBytes = SECT_SIZE;

			if( !fe->sctNum )
			{
				fe->sctBegin = SCT_END_FLAG;
			}

			ret = 1;
		}
	}

	return ret;
}

static uint EraseLast(FSRoot* fe, uint bytes)
{
	uint ret = 0;

	while( fe->sctNum && bytes )
	{
		//判断需要删除的长度
		if( bytes < fe->lastBytes )
		{
			fe->lastBytes -=bytes;

			ret += bytes;

			bytes = 0;
		}
		else
		{
			bytes -= fe->lastBytes;

			ret += fe->lastBytes;

			fe->lastBytes = 0;

			AdjustStorage(fe);
		}
	}

	return ret;
}

static uint DeleteInRoot(const char* name)
{
	FSRoot* root = (FSRoot*)ReadSector(ROOT_SCT_IDX);
	FileEntry* fe = FindInRoot(name); //找到需要删除文件的扇区分配信息
	uint ret = 0;

    if( root && fe )
    {
		uint last = FindLast(root->sctBegin);
		//找到目标和最后一个
		FileEntry* feTarget = ReadSector(fe->inSctIdx);
		FileEntry* feLast = (last != SCT_END_FLAG) ? ReadSector(last) : NULL;

		if( feTarget && feLast )
		{
			uint lastOff = root->lastBytes / FE_BYTES - 1;
			FileEntry* lastItem = AddrOff(feLast, lastOff);
			FileEntry* targetItem = AddrOff(feTarget, fe->inSctOff);

			FreeFile(targetItem->sctBegin);

			MoveFileEntry(targetItem, lastItem);

			EraseLast(root, FE_BYTES);

			ret = HDRawWrite(ROOT_SCT_IDX, (byte *) root) &&
				    HDRawWrite(fe->inSctIdx, (byte *) feTarget);
		}

		Free(feTarget);
		Free(feLast);
    }

	Free(root);
	Free(fe);

	return ret;
}

uint FOpen(const char* fn)
{
    FileDesc* ret = NULL;

		if( fn && !IsOpened(fn) )
		{
		   FileEntry* fe = NULL;

		   ret = (FileDesc*)Malloc(FD_BYTES); //分配空间
		   fe = ret ? FindInRoot(fn) : NULL;  //按文件名在根目录进行查找

		   if( ret && fe )
		   {
		      ret->fe = *fe;  //赋值对应的文件描述符信息 浅拷贝
		      ret->objIdx = SCT_END_FLAG;
		      ret->offset = SECT_SIZE;
		      ret->changed = 0;

		      List_Add(&gFDList, (ListNode *)ret);
		   }

		   Free(fe);
		}

		return (uint)ret;
}

static uint IsFDValid(FileDesc* fd)
{
   uint ret = 0;
   ListNode* pos = NULL;

   //遍历链表查看是否是已经打开的文件
   List_ForEach(&gFDList, pos)
   {
      if( IsEqual(pos, fd) )
      {
        ret = 1;
        break;
      }
   }

   return ret;
}

static uint FlushCache(FileDesc* fd)
{
   uint ret = 1;

   if( fd->changed )
   {
      uint sctIdx = FindIndex(fd->fe.sctBegin, fd->objIdx);

      ret = 0;

      //将数据写入到对应扇区内
      if( (sctIdx != SCT_END_FLAG) && (ret = HDRawWrite(sctIdx, fd->cache)))
      {
          fd->changed = 0;
      }
      
   }

   return ret;
}

static uint FlushFileEntry(FileEntry* fe)
{
	uint ret = 0;
	FileEntry* feBase = ReadSector(fe->inSctIdx);
	FileEntry* feInSct = AddrOff(feBase, fe->inSctOff);

	*feInSct = *fe;  //赋值

	ret = HDRawWrite(fe->inSctIdx, (byte *) feBase); //写入硬盘

	Free(feBase);

	return ret;
}

static uint ToFlush(FileDesc* fd)
{
   //分别是写入到数据区和扇区管理区
   return FlushCache(fd) && FlushFileEntry(&fd->fe);
}

void FClose(uint fd)
{
   FileDesc* pf = (FileDesc*)fd;

   if( IsFDValid(pf) )
   {
      ToFlush(pf);

      List_DelNode((ListNode *) pf);

      Free(pf);
   }
}

static uint ReadToCache(FileDesc* fd, uint idx)
{
   uint ret = 0;

   if( idx < fd->fe.sctNum )
   {
      uint sctIdx = FindIndex(fd->fe.sctBegin, idx);

      ToFlush(fd);  //将当前cache中的内容写回到硬盘中

      //重新从硬盘中读取新扇区到cache
      if( (sctIdx != SCT_END_FLAG) && (ret = HDRawRead(sctIdx, fd->cache) ) )
      {
         fd->objIdx = idx;
         fd->offset = 0;
         fd->changed = 0;
      }
   }

   return ret;
}

static uint PrepareCache(FileDesc* fd, uint objIdx)
{
    CheckStorage(&fd->fe); //判断是否需要分配一个新扇区

    return ReadToCache(fd, objIdx);
}

static uint CopyToCache(FileDesc* fd, byte* buf, uint len)
{
    uint ret = -1;

    if( fd->objIdx != SCT_END_FLAG )
    {
       uint n = SECT_SIZE - fd->offset;  
       byte* p = AddrOff(fd->cache, fd->offset);

       n = (n < len) ? n : len; //计算当前cache还可以写多长数据

       MemCpy(p, buf, n);

       fd->offset += n;
       fd->changed = 1;

       //假设是当前写入的是最后一个扇区则设置一下 lastBytes
       if( ((fd->fe.sctNum - 1) == fd->objIdx) && (fd->fe.lastBytes < fd->offset) )
       {
           fd->fe.lastBytes = fd->offset;
       }

       ret = n;
    }

    return ret;
}

static uint ToWrite(FileDesc* fd, byte* buf, uint len)
{
    uint ret = 1;
    uint i = 0;
    uint n = 0;

    while( (i < len) && ret )
    {
       byte* p = AddrOff(buf, i);

       if( fd->offset == SECT_SIZE )  //如果当前扇区已经写满
       {
         ret = PrepareCache(fd, fd->objIdx + 1);
       }

       if( ret )
       {
          n = CopyToCache(fd, p, len - i);

          i += n;
       }
    }

    ret = i;  //返回值是写入的数据长度

    return ret;
}

uint FWrite(uint fd, byte* buf, uint len)
{
   uint ret = -1;

   if( IsFDValid((FileDesc *) fd) && buf )
   {
      ret = ToWrite((FileDesc*)fd, buf, len);
   }

   return ret;
}

uint FDelete(const char* fn)
{
	return fn && !IsOpened(fn) && DeleteInRoot(fn) ? FS_SUCCEED : FS_FAILED;
}

uint FSFormat()
{
	FSHeader* header = (FSHeader*)Malloc(SECT_SIZE);
	FSRoot* root = (FSRoot*)Malloc(SECT_SIZE);
	uint* p = (uint*)Malloc(MAP_ITEM_CNT * sizeof(uint));
    uint ret = 0;

	if( header && root && p )
	{
		uint i = 0;
		uint j = 0;
		uint current = 0;

		StrCpy(header->magic, FS_MAGIC, sizeof(header->magic) - 1);
		header->sctNum = HDRawSectors(); //获得硬盘的总扇区数
		//扇区分配表个数 一个扇区为512字节 扇区分配单元占4字节  
		header->mapSize = (header->sctNum - FIXED_SCT_SIZE) / 129 + !!((header->sctNum - FIXED_SCT_SIZE) % 129);
		
		//空闲扇区个数计算 总扇区 - 扇区分配表个数 - 固定扇区个数
		header->freeNum = header->sctNum - header->mapSize - FIXED_SCT_SIZE; 
		
		//空闲扇区起始位置计算 固定扇区 + 扇区分配表个数
		header->freeBegin = FIXED_SCT_SIZE + header->mapSize;

		ret = HDRawWrite(HEADER_SCT_IDX, (byte *) header);

		StrCpy(root->magic, ROOT_MAGIC, sizeof(root->magic)-1);

		root->sctNum = 0;
		root->sctBegin = SCT_END_FLAG;
        root->lastBytes = SECT_SIZE;

		ret = ret && HDRawWrite(ROOT_SCT_IDX, (byte*)root);
		
        //初始化扇区分配表
		for(i=0; ret && (i < header->mapSize) && (current < header->freeNum); i++)
		{
			//一次操作整个扇区
			for(j=0; j<MAP_ITEM_CNT; j++)
			{
				uint* pInt = AddrOff(p, j); 

                //给扇区分配单元设置对应哪一个扇区 扇区数不能超过空闲扇区数
				if( current < header->freeNum )
				{
					*pInt = current + 1;  

					if( current == (header->freeNum - 1) )
					{
						*pInt = SCT_END_FLAG;
					}

					current ++;					
				}
				else
				{
					break;
				}
			}

			ret = ret && HDRawWrite(i + FIXED_SCT_SIZE, (byte*)p); //将信息写入到硬盘
		}
	}

	Free(header);
	Free(root);
	Free(p);

	return ret;	
}

uint FSIsFormatted()
{
	//读取扇区
	FSHeader* header = (FSHeader*)ReadSector(HEADER_SCT_IDX);
	FSRoot* root = (FSRoot*)ReadSector(ROOT_SCT_IDX);
    uint ret = 0;

    if( header && root )
    {
		ret = StrCmp(header->magic, FS_MAGIC, -1) &&
			    (header->sctNum == HDRawSectors()) &&
			    StrCmp(root->magic, ROOT_MAGIC, -1);
    }

	Free(header);
	Free(root);

	return ret;		
}

uint FRename(const char* ofn, const char* nfn)
{
	uint ret = FS_FAILED;

	if( ofn && !IsOpened(ofn) && nfn )
	{
		FileEntry* ofe = FindInRoot(ofn);
		FileEntry* nfe = FindInRoot(nfn);

		if( ofe && !nfe )
		{
			StrCpy(ofe->name, nfn, sizeof(ofe->name) - 1);

			if( FlushFileEntry(ofe) )
			{
				ret = FS_SUCCEED;
			}
		}

		Free(ofe);
		Free(nfe);
	}

	return ret;
}

static uint GetFileLen(FileDesc* fd)
{
   uint ret = 0;

   if( fd->fe.sctBegin != SCT_END_FLAG )
   {
      ret = (fd->fe.sctNum - 1) * SECT_SIZE + fd->fe.lastBytes; //先计算完整扇区的大小 再加上最后一个扇区的bytes大小
   }

   return ret;
}

static uint GetFilePos(FileDesc* fd)
{
   uint ret = 0;

   if( fd->objIdx != SCT_END_FLAG )
   {
      ret = fd->objIdx * SECT_SIZE + fd->offset; //计算当前在文件的哪个位置
   }

   return ret;
}

static uint CopyFromCache(FileDesc* fd, byte* buf, uint len)
{
   uint ret = (fd->objIdx != SCT_END_FLAG);

   if( ret )
   {
      uint n = SECT_SIZE - fd->offset;
      byte* p = AddrOff(fd->cache, fd->offset);

      n = (n < len) ? n : len;  //计算本次拷贝还能拷多少长度数据

      MemCpy(buf, p, n);

      fd->offset += n;

      ret = n;
   }

   return ret;
}

static uint ToRead(FileDesc* fd, byte* buf, uint len)
{
   uint ret = -1;
   uint n = GetFileLen(fd) - GetFilePos(fd);
   uint i = 0;

   len = (len < n) ? len : n;

   while( (i < len) && ret )
   {
      byte* p = AddrOff(buf, i);

      if( fd->offset == SECT_SIZE )  //如果当前读写指针超过一个扇区 则读取下一个扇区
      {
         ret = PrepareCache(fd, fd->objIdx + 1);
      }

      if( ret )
      {
         n = CopyFromCache(fd, buf, len - i); //拷贝剩余部分
      }

      i += n;
   }

   ret = i;

   return ret;
}

uint FRead(uint fd, byte* buf, uint len)
{
    uint ret = -1;

    if( IsFDValid((FileDesc*)fd) && buf)
    {
      ret = ToRead((FileDesc*)fd, buf, len); 
    }

    return ret;
}

void listFile()
{
    FSRoot* root = (FSRoot*)ReadSector(ROOT_SCT_IDX);
    FileEntry* feBase = (FileEntry*)ReadSector(root->sctBegin);
    int i = 0;
    int n = 0;
	
    printf("sctNum = %d\n", root->sctNum);
    printf("lastBytes = %d\n", root->lastBytes);
	
    n = root->lastBytes / FE_BYTES;
    for(i=0; i<n; i++)
    {
        FileEntry* fe = AddrOff(feBase, i);
        printf("name = %s\n", fe->name);
    }
}

//重新设置读写指针的位置
static uint ToLocate(FileDesc* fd, uint pos)
{
   uint ret = -1;
   uint len = GetFileLen(fd);

   pos = (pos < len) ? pos : len;

	 uint objIdx = pos / SECT_SIZE;
	 uint offset = pos % SECT_SIZE;
	 uint sctIdx = FindIndex(fd->fe.sctBegin, objIdx);

	 ToFlush(fd);

	 if( (sctIdx != SCT_END_FLAG ) && HDRawRead(sctIdx, fd->cache) )
	 {
        fd->objIdx = objIdx;
        fd->offset = offset;

        ret = pos;	    
	 }

   return ret; 
}

uint FErase(uint fd, uint bytes)
{
   uint ret = 0;
   FileDesc* pf = (FileDesc*)fd;

   if( IsFDValid(pf) )
   {
      uint pos = GetFilePos(pf);
      uint len = GetFileLen(pf);

      ret = EraseLast(&pf->fe, bytes); //擦除后面的字节

      len -= ret;

      if( ret && (pos > len) )
      {
          ToLocate(pf, len);
      }
   }
}

uint FSeek(uint fd, uint pos)
{
    uint ret = -1;
    FileDesc* pf = (FileDesc*)fd;

    if( IsFDValid(pf) )
    {
        ret = ToLocate(pf, pos);
    }

    return ret;  
}

uint FLength(uint fd)
{
    uint ret = -1;
    FileDesc* pf = (FileDesc*)fd;

    if( IsFDValid(pf) )
    {
        ret = GetFileLen(pf);
    }

    return ret;
}

uint FTell(uint fd)
{
    uint ret = -1;
    FileDesc* pf = (FileDesc*)fd;

    if( IsFDValid(pf) )
    {
        ret = GetFilePos(pf);
    }

    return ret;
}

uint FFlush(uint fd)
{
    uint ret = -1;
    FileDesc* pf = (FileDesc*)fd;

    if( IsFDValid(pf) )
    {
        ret = ToFlush(pf);
    }

    return ret;
}