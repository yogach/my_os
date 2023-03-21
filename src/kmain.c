#include "kernel.h"
#include "screen.h"
#include "global.h"

Task* gCTaskAddr = NULL;
Task p = {0};
void (* const InitInterrupt)() = NULL;
void (* const EnableTimer)() = NULL;
void (* const SendEOI)() = NULL;
void TimerHandlerEntry();

void Delay(int n)
{
    while( n > 0 )
    {
       int i=0;
       int j=0;
       
       for(i=0; i<1000; i++)
        {
            for(j=0; j<1000; j++)
            {
              asm volatile ("nop \n");
            }
        }
        
        n --;
    }
}

void TaskA()
{
    int i = 0;
    
    SetPrintPos(0, 12);
    
    PrintString("Task A: ");
    
    while(1)
    {
        SetPrintPos(8, 12);
        PrintChar('A' + i);
        i = (i + 1) % 26;
        Delay(1);
    }
}

void TimerHandler()
{
    static uint i = 0;
    
    i = (i + 1) % 10;
    
    SetPrintPos(0, 13);
        
    PrintString("Timer: ");
    
    if( i == 0 )
    {
        static uint j = 0;
        
        SetPrintPos(0, 13);
        
        PrintString("Timer: ");
        
        SetPrintPos(8, 13);
        
        PrintIntDec(j++);
    }
    
    SendEOI(MASTER_EOI_PORT); //发送中断结束标志
}

void KMain()
{
    int n = PrintString("D.T.OS\n");
    uint base = 0;
    uint limit = 0;
    ushort attr = 0;
    int i = 0;
    
    PrintString("GDT Entry: ");
    PrintIntHex((uint)gGdtInfo.entry);
    PrintChar('\n');
    
    for(i=0; i<gGdtInfo.size; i++)
    {
        GetDescValue(gGdtInfo.entry + i, &base, &limit, &attr);
       
        PrintIntHex(base);
        PrintString("    ");
    
        PrintIntHex(limit);
        PrintString("    ");
    
        PrintIntHex(attr);
        PrintChar('\n');       
    }
    
    PrintString("RunTask: ");
    PrintIntHex((uint)RunTask);
    PrintChar('\n');
    
    //设置任务相关寄存器
    p.rv.cs = LDT_CODE32_SELECTOR;
    p.rv.gs = LDT_VIDEO_SELECTOR;
    p.rv.ds = LDT_DATA32_SELECTOR;
    p.rv.es = LDT_DATA32_SELECTOR;
    p.rv.fs = LDT_DATA32_SELECTOR;
    p.rv.ss = LDT_DATA32_SELECTOR;   //ss-栈段寄存器
    
    p.rv.esp = (uint)p.stack + sizeof(p.stack); //esp 栈顶指针寄存器 指向预定义的栈顶
    p.rv.eip = (uint)TaskA;         // eip 下一条指令地址指向任务入口
    p.rv.eflags = 0x3202;     //IOPL = 3 if = 0
    
    p.tss.ss0 = GDT_DATA32_FLAT_SELECTOR;    //设置0特权级的栈
    p.tss.esp0 = (uint)&p.rv + sizeof(p.rv); //esp指针指向 Task结构体RegValue的末尾 目的是在调用中断时自动完成ss esp eflags cs的压栈 
    p.tss.iomb = sizeof(p.tss);
    
    //设置局部段描述符
    SetDescValue(p.ldt + LDT_VIDEO_INDEX, 0xB8000, 0x07FFF, DA_DRWA + DA_32 + DA_DPL3);
    SetDescValue(p.ldt + LDT_CODE32_INDEX, 0x00,    0xFFFFF, DA_C + DA_32 + DA_DPL3);
    SetDescValue(p.ldt + LDT_DATA32_INDEX, 0x00,    0xFFFFF, DA_DRW + DA_32 + DA_DPL3);
    
    //设置选择子
    p.ldtSelector = GDT_TASK_LDT_SELECTOR;
    p.tssSelector = GDT_TASK_TSS_SELECTOR;
    
    //设置全局段描述符
    SetDescValue(&gGdtInfo.entry[GDT_TASK_LDT_INDEX], (uint)&p.ldt, sizeof(p.ldt)-1, DA_LDT + DA_DPL0);
    SetDescValue(&gGdtInfo.entry[GDT_TASK_TSS_INDEX], (uint)&p.tss, sizeof(p.tss)-1, DA_386TSS + DA_DPL0);
    
    SetIntHandler(gIdtInfo.entry + 0x20, (uint)TimerHandlerEntry);  //设置中断处理函数
    
    InitInterrupt();
    
    EnableTimer();
    
    gCTaskAddr = &p;
    
    //启动任务
    RunTask(&p);    
}
