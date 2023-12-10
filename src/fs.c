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
