
#include "kernel.h"

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

int SetIntHandler(Gate* pGate, uint ifunc)
{
   int ret = 0;
   
   if( ret = (pGate != NULL) )
   {
      pGate->offset1  = ifunc & 0xffff;
      pGate->selector = GDT_CODE32_FLAT_SELECTOR; //设置选择子为平坦内存选择子
      pGate->dcount   = 0;
      pGate->attr     = DA_386IGate + DA_DPL0;
      pGate->offset2  = (ifunc >> 16) & 0xffff;    
   }
   
   return ret;
}

int GetIntHandler(Gate* pGate, uint* pIFunc)
{
   int ret = 0;
   
   if( ret = (pGate && pIFunc) )
   {
      *pIFunc = (pGate->offset2 << 16) | pGate->offset1;
   }
   
   return ret;
}
