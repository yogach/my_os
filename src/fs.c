#include "hdraw.h"
#include "fs.h"
#include "utility.h"

#ifdef DTFSER
  #include <malloc.h>
  #define Malloc malloc
  #define Free free
#else
  #include "memory.h"
#endif

#define FS_MAGIC       "DTFS-v1.0"
#define ROOT_MAGIC     "ROOT"
#define HEADER_SCT_IDX 0                 //头信息扇区号
#define ROOT_SCT_IDX   1                 //根信息扇区号
#define FIXED_SCT_SIZE 2                 //固定扇区占用数
#define SCT_END_FLAG   ((uint)-1)
#define FE_BYTES       sizeof(FileEntry)
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
	uint* pSct;   //扇区分配单元对应扇区的数据
	uint sctIdx;  //绝对扇区号
	uint sctOff;  //扇区分配表的第几个扇区
	uint idxOff;  //对应扇区的第几个扇区分配单元
} MapPos;

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

            *pInt = header->freeBegin - FIXED_SCT_SIZE - header->mapSize; //扇区分配单元内存放的是相对位置

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
				ret = *pInt + header->mapSize + FIXED_SCT_SIZE;
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

//找到第idx个扇区
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

//标记扇区
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

static uint CheckStorge(FSRoot* fe)
{
	uint ret = 0;

    //判断扇区是否已经被写满了
	if( fe->lastBytes == SECT_SIZE )
	{
		uint si = AllocSector();  //分配一个扇区

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

	if( (last != SCT_END_FLAG) && (feBase == (FileEntry*)ReadSector(last)) )
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
	

	return ret;
}

static uint CreateInRoot(const char* name)
{
	FSRoot* root = (FSRoot*)ReadSector(ROOT_SCT_IDX);
	uint ret = 0;

    if( root )
    {
		CheckStorge(root);

		if( CreateFileEntry(name, root->sctBegin, root->lastBytes) )
		{
			root->lastBytes += FE_BYTES;

			ret = HDRawWrite(ROOT_SCT_IDX, (byte*)root);
		}
    }

	Free(root);

	return ret;
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
				*ret = *fe; //结构体赋值可以这样？
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

    //先查找最后一个扇区前面的所有扇区 因为需要查找全部
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

    //查找完最后一个扇区前面的所有扇区后 
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
	FSRoot* root = (FSRoot*)ReadSector(ROOT_SCT_IDX);
	FileEntry* ret = NULL;

	if( root && root->sctNum )
	{
		ret = FindFileEntry(name, root->sctBegin, root->sctNum, root->lastBytes);
	}

    Free(root);
	
	return ret;
}

uint FCreate(const char* fn)
{
	
}

uint FExisted(const char* fn)
{
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
