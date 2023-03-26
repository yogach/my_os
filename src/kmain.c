#include "screen.h"
#include "task.h"
#include "interrupt.h"

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
    
    PrintString("RunTask: ");
    PrintIntHex((uint)RunTask);
    PrintChar('\n');
    
    PrintString("LoadTask: ");
    PrintIntHex((uint)LoadTask);
    PrintChar('\n');
    
    IntModInit();
    
    TaskModInit();
    
    LaunchTask();    
}
