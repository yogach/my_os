#include "interrupt.h"
#include "task.h"

extern volatile Task* gCTaskAddr;

void TimerHandler()
{
    static uint i = 0;
    
    i = (i + 1) % 5;  
    if( i == 0 )
    {
       Schedule();
    }
    
    SendEOI(MASTER_EOI_PORT); //发送中断结束标志
}

void SysCallHandler(ushort ax)
{
	  if( ax == 0 )
	  {
		KillTask();
	  }
}

void PageFaultHandler()
{
    SetPrintPos(0, 6);
    
    PrintString("Page Fault: kill ");
    PrintString(gCTaskAddr->name);
    
    KillTask();
}