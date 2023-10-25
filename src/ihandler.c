#include "interrupt.h"
#include "task.h"
#include "keyboard.h"
#include "sysinfo.h"

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
		 case 2:
		 	KeyCallHandler(cmd, param1, param2);
		 	break;
		 case 3:
		 	SysInfoCallHandler(cmd, param1, param2);
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
    byte sc = ReadPort(0x60); //读取键盘按键值

    PutScanCode(sc);

/*
    uint code = 0;

    while( code = FetchKeyCode() )
    {      
       if( (((byte)(code >>8)) == 0x13) && (code & 0x1000000) )
       {
          PrintString("Pause Pressed\n");
       }
       else if(((byte)(code >>8)) == 0x13)
       { 
          PrintString("Pause Released\n");
       }

       if( (char)code && (code & 0x1000000) )
       {
         PrintChar((char)code);
       }
       else if( (((byte)(code >>8)) == 0x0D) && (code & 0x1000000) )
       {
         PrintChar('\n');
       }
    }
*/    
    NotifyKeyCode();  //在读取到按键输入后 重新将等待按键的任务放入执行队列

    SendEOI(MASTER_EOI_PORT);
}