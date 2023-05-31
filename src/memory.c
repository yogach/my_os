
#include "memory.h"
#include "utility.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define FM_ALLOC_SIZE   32
#define FM_NODE_SIZE    sizeof(FMemNode)

typedef byte(FMemUnit)[FM_ALLOC_SIZE];  //声明一个数组 别名为FMemUnit
typedef union _FMemNode  FMemNode;

union _FMemNode
{
	 FMemNode* next;
	 FMemUnit* ptr;
};

typedef struct
{
	FMemNode* node;
	FMemNode* nbase; //管理单元的起始地址
	FMemUnit* ubase; //被分配空间的起始地址
	uint max;
} FMemList;

static FMemList gFMemList = {0};

static void FMemInit(byte* mem, uint size)
{
	FMemNode* p = NULL;
	int i = 0;
	uint max = 0;

	max = size / (FM_NODE_SIZE + FM_ALLOC_SIZE); //计算被分配空间的总块数

	gFMemList.max = max;
	gFMemList.nbase = (FMemNode*)mem;
	gFMemList.ubase = (FMemUnit*)((uint)mem + max * FM_ALLOC_SIZE);
	gFMemList.node = (FMemNode*)mem;

	p = gFMemList.node;

  //建立管理单元之间链表
	for(i=0; i<max-1; i++)
	{
		FMemNode* current = (FMemNode*)AddrOff(p, i);
		FMemNode* next = (FMemNode*)AddrOff(p, i+1);

		current->next = next;
	}

	((FMemNode*)AddrOff(p, i))->next = NULL;
}

static void* FMemAlloc()
{
	void* ret = NULL;

	if( gFMemList.node )
	{
		FMemNode* alloc = gFMemList.node;              //得到首节点
		int index = AddrIndex(alloc, gFMemList.nbase); //得到首节点是管理单元的第几块

		ret = AddrOff(gFMemList.ubase, index);    //得到分配区块地址

		gFMemList.node = alloc->next;  //首节点指向下一个节点

		alloc->ptr = ret;              //将得到的区块地址设置到ptr里 由于union的特性 next地址也被改变
	}

	return ret;
}

static int FMemFree(void* ptr)
{
	int ret = 0;

  if( ptr )
  {
		uint index = AddrIndex((FMemUnit*)ptr, gFMemList.ubase);
		FMemNode* node = AddrOff(gFMemList.nbase, index);

    //判断需要释放的地址是否和node中保存的值相等
		if( (index < gFMemList.max) && IsEqual(ptr, node->ptr) )
		{
			//将释放之后的节点重新排到链表起始位置
			node->next = gFMemList.node;
			
			gFMemList.node = node;

			ret = 1;
		}
  }

	return ret;
}

void fmem_test()
{
	static byte fmem[0x10000] = {0};
	static void* array[2000] = {0};
	int i = 0;

	FMemNode* pos = NULL;

	FMemInit(fmem, sizeof(fmem));

	pos = gFMemList.node;

	while( pos )
	{
		i++;
		pos = pos->next;
	}

	printf("i = %d\n", i++);
	
  //随机进行alloc free
	for(i=0; i<100000; i++)
	{
			int ii = i % 2000;
			byte* p = FMemAlloc();
			
			if( array[ii] )
			{
					FMemFree(array[ii]);
					
					array[ii] = NULL; 
			}
			
			array[ii] = p;
			
			if( i % 3 == 0 )
			{
					int index = rand() % 2000;
					
					FMemFree(array[index]);
					
					array[index] = NULL;
			}
	}
	
	for(i=0; i<2000; i++)
	{
			FMemFree(array[i]);
	}
	
	i = 0;
	
	pos = gFMemList.node;
	
	while( pos )
	{
			i++;
			pos = pos->next;
	}
	
	printf("i = %d\n", i++);

	
}


