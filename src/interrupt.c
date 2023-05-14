#include "interrupt.h"
#include "ihandler.h"
#include "utility.h"

void (* const InitInterrupt)() = NULL;
void (* const EnableTimer)() = NULL;
void (* const SendEOI)(uint port) = NULL;

void IntModInit()
{
   SetIntHandler(AddrOff(gIdtInfo.entry, 0x0e), (uint)PageFaultHandlerEntry);  //设置页错误异常处理函数
   SetIntHandler(AddrOff(gIdtInfo.entry, 0x20), (uint)TimerHandlerEntry);  //设置定时器中断处理函数
   SetIntHandler(AddrOff(gIdtInfo.entry, 0x80), (uint)SysCallHandlerEntry);  //设置系统调用软中断处理函数
	 
   InitInterrupt();
    
   //EnableTimer();   
}

int SetIntHandler(Gate* pGate, uint ifunc)
{
   int ret = 0;
   
   if( ret = (pGate != NULL) )
   {
      pGate->offset1  = ifunc & 0xffff;
      pGate->selector = GDT_CODE32_FLAT_SELECTOR; //设置选择子为平坦内存选择子
      pGate->dcount   = 0;
      pGate->attr     = DA_386IGate + DA_DPL3;   //定义在特权级3下也可调用中断 主要是软中断
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
