#include "screen.h"
#include "task.h"
#include "interrupt.h"
#include "app.h"

void KMain()
{
    int n = PrintString("D.T.OS\n");
    
    PrintString("GDT Entry: ");
    PrintIntHex((uint)gGdtInfo.entry);
    PrintChar('\n');
    
    PrintString("GDT Size: ");
    PrintIntDec((uint)gGdtInfo.size);
    PrintChar('\n');
    
    PrintString("IDT Entry: ");
    PrintIntHex((uint)gIdtInfo.entry);
    PrintChar('\n');
    
    PrintString("IDT Size: ");
    PrintIntDec((uint)gIdtInfo.size);
    PrintChar('\n');

		AppModInit();
    
    TaskModInit();
    
    IntModInit(); //初始化中断    
    
    LaunchTask(); //开始执行任务
}
