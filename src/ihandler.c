#include "interrupt.h"
#include "task.h"

extern byte ReadPort(ushort port);

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

void SysCallHandler(uint type, uint cmd, uint param1, uint param2 ) // __cdecl__
{
   switch(type)
   {
	 	 case 0:
		 	TaskCallHandler(cmd, param1, param2);
			break;
		 case 1:
		 	MutexCallHandler(cmd, param1, param2);
			break;
		 default:
		 	break;
   }
}

void PageFaultHandler()
{
    SetPrintPos(0, 6);
    
    PrintString("Page Fault: kill ");
    PrintString(gCTaskAddr->name);
    
    KillTask();
}

void SegmentFaultHandler()
{
    SetPrintPos(0, 6);
    
    PrintString("Segment Fault: kill ");
    PrintString(gCTaskAddr->name);
    
    KillTask();
}

void KeyboardHandler()
{
    byte kc = ReadPort(0x60); //读取键盘按键值

    PrintIntHex(kc);
    PrintChar(' ');
    
    SendEOI(MASTER_EOI_PORT);
}