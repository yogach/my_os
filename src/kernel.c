
#include "kernel.h"

GdtInfo gGdtInfo = {0};
IdtInfo gIdtInfo = {0};

int SetDescValue(Descriptor* pDesc, uint base, uint limit, ushort attr)
{
   int ret = 0;
   
   if( ret = (pDesc != NULL) )
   {
     pDesc->limit1    = limit & 0xffff;
     pDesc->base1     = base & 0xffff;
     pDesc->base2     = (base >> 16) & 0xff;
     pDesc->attr1     = attr & 0xff;
     pDesc->attr2_limit2 = ((attr >> 8) & 0xf0) | ((limit >> 16) & 0x0f);
     pDesc->base3     = (base >> 24) & 0xff;
   }
   
   return ret;
}

int GetDescValue(Descriptor* pDesc, uint* pBase, uint* pLimit, ushort* pAttr)
{
   int ret = 0;
   
   if( ret = (pDesc && pBase && pLimit && pAttr) )
   {
     *pBase = (pDesc->base3 << 24) | (pDesc->base2 << 16) | pDesc->base1;
     *pLimit = ((pDesc->attr2_limit2 & 0xf) << 16) | pDesc->limit1;
     *pAttr = ((pDesc->attr2_limit2 & 0xf0) << 8) | pDesc->attr1;
   }
   
   return ret;
}

void ConfigPageTable()
{
	uint* TblBase = (void*)PageTblBase;
	//根据分页机制 得到BaseOfApp的地址 在页表的BaseOfApp / 0x1000页 减一的目的是为了得到前一页的地址
	uint index = BaseOfApp / 0x1000 - 1;  
	int i = 0;

  //从第0页开始修改属性
	for(i=0x0; i<=index; i++)
	{
		uint* addr = TblBase + i; 
		uint value = *addr;       //取出地址内容

        value = value & 0xFFFFFFFD;  //将页属性的RW位修改为0 即特权级3不能对这个页进行修改

		*addr = value;		
	}
	
}


