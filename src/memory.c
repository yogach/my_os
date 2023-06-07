
#include "memory.h"
#include "utility.h"
#include "list.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define FM_ALLOC_SIZE   32
#define FM_NODE_SIZE    sizeof(FMemNode)
#define VM_HEAD_SIZE    sizeof(VMemHead)

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

typedef struct 
{
	ListNode head;
  uint used;
	uint free;
	byte* ptr;
}VMemHead;

static FMemList gFMemList = {0};
static List gVMemList = {0};     //声明链表头

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

static void VMemInit(byte* mem, uint size)
{
	List_Init((List*)&gVMemList);
	VMemHead* head = (VMemHead*)mem;

	head->used = 0;
	head->free = size - VM_HEAD_SIZE;  //空闲大小需要减去VMemHead结构体大小
	head->ptr = AddrOff(head, 1);      //可用空间地址从管理数据结构之后开始

	List_Add(&gVMemList, (ListNode*) head);
}

static void* VMemAlloc(uint size)
{
	ListNode* pos = NULL;
	VMemHead* ret = NULL;

	uint alloc = size + VM_HEAD_SIZE;

  //遍历链表节点
	List_ForEach(&gVMemList, pos)
	{
		VMemHead* current = (VMemHead*)pos;

    //找到空闲大小 大于等于 需分配大小的节点
		if( current->free >= alloc )
		{
			//从当前空闲空间的末尾 开始分配
			byte* mem = (byte*)((uint)current->ptr + (current->free + current->used) - alloc);

			ret = (VMemHead*)mem;

			ret->free = 0;
			ret->used = size;
			ret->ptr = AddrOff(ret, 1);

			current->free -= alloc;

      //将分配的节点插入到当前节点的后面
			List_AddAfter((ListNode*)current, (ListNode*)ret); 

			break;
			
		}
	}

  //判断是否分配到空间 分配到则返回可用空间地址
	return ret ? ret->ptr : NULL;
}

static int VMemFree(void* ptr)
{
	int ret = 0;

	if( ptr )
	{
		ListNode* pos = NULL;

		List_ForEach(&gVMemList, pos)
		{
			VMemHead* current = (VMemHead*)pos;

			if( IsEqual(current->ptr, ptr) )
			{
				 VMemHead* prev = (VMemHead*)(current->head.prev); //得到前一个链表节点

         //将current节点内的所有空间都加到前一个节点中 
				 prev->free += current->used + current->free + VM_HEAD_SIZE;

         //删除当前节点
				 List_DelNode((ListNode*) current);

				 ret = 1;

				 break;
			}
		}
	}

	return ret;
}

void MemModInit(byte* mem, uint size)
{
	byte* fmem = mem;
	uint fsize = size / 2;  //内存分为两份
	byte* vmem = AddrOff(fmem, fsize);
	uint vsize = size - fsize;

	FMemInit(fmem, fsize);
	VMemInit(vmem, vsize);
}

void* Malloc(uint size)
{
	void* ret = NULL;

  //根据需要空间的大小 选择不同的分配方法
  if( size <= FM_ALLOC_SIZE )
  {
		ret = FMemAlloc();
  }

	if( ret == NULL )
	{
		ret = VMemAlloc(size);
	}

	return ret;
}

void Free(void* ptr)
{
	if( ptr )
	{
		//首先尝试使用定长内存释放 如果失败则使用变长内存释放
		if( !FMemFree(ptr) )
		{
			VMemFree(ptr);
		}
	}
}


