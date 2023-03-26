#include "utility.h"
#include "task.h"

void (* const RunTask)(volatile Task* pt) = NULL;
void (* const LoadTask)(volatile Task* pt) = NULL;

volatile Task* gCTaskAddr = NULL; //使用volatile 防止编译器自动优化此变量 造成指针的值没有变化
Task p = {0};
Task t = {0};
TSS gTSS = {0};

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

void TaskB()
{
    int i = 0;
    
    SetPrintPos(0, 13);
    
    PrintString("Task B: ");
    
    while(1)
    {
        SetPrintPos(8, 13);
        PrintChar('0' + i);
        i = (i + 1) % 10;
        Delay(1);
    }
}



static void InitTask(Task* pt, void(*entry)())
{
    //设置任务相关寄存器
    pt->rv.cs = LDT_CODE32_SELECTOR;
    pt->rv.gs = LDT_VIDEO_SELECTOR;
    pt->rv.ds = LDT_DATA32_SELECTOR;
    pt->rv.es = LDT_DATA32_SELECTOR;
    pt->rv.fs = LDT_DATA32_SELECTOR;
    pt->rv.ss = LDT_DATA32_SELECTOR;   //ss-栈段寄存器
    
    pt->rv.esp = (uint)pt->stack + sizeof(pt->stack); //esp 栈顶指针寄存器 指向预定义的栈顶
    pt->rv.eip = (uint)entry;         // eip 下一条指令地址指向任务入口
    pt->rv.eflags = 0x3202;     //IOPL = 3 if = 1
    
    gTSS.ss0 = GDT_DATA32_FLAT_SELECTOR;    //设置0特权级的栈
    gTSS.esp0 = (uint)&pt->rv + sizeof(pt->rv); //esp指针指向 Task结构体RegValue的末尾 目的是在调用中断时自动完成ss esp eflags cs的压栈 
    gTSS.iomb = sizeof(TSS);
    
    //设置局部段描述符
    SetDescValue(AddrOff(pt->ldt, LDT_VIDEO_INDEX),  0xB8000, 0x07FFF, DA_DRWA + DA_32 + DA_DPL3);
    SetDescValue(AddrOff(pt->ldt, LDT_CODE32_INDEX), 0x00,    0xFFFFF, DA_C + DA_32 + DA_DPL3);
    SetDescValue(AddrOff(pt->ldt, LDT_DATA32_INDEX), 0x00,    0xFFFFF, DA_DRW + DA_32 + DA_DPL3);
    
    //设置选择子
    pt->ldtSelector = GDT_TASK_LDT_SELECTOR;
    pt->tssSelector = GDT_TASK_TSS_SELECTOR;
    
    //设置全局段描述符
    SetDescValue(AddrOff(gGdtInfo.entry, GDT_TASK_LDT_INDEX), (uint)&pt->ldt, sizeof(pt->ldt)-1, DA_LDT + DA_DPL0);
    SetDescValue(AddrOff(gGdtInfo.entry, GDT_TASK_TSS_INDEX), (uint)&gTSS, sizeof(gTSS)-1, DA_386TSS + DA_DPL0);
}

void TaskModInit()
{
    //这里有差别 初始化的顺序 会实际影响到gTSS的值 首个启动的任务必须时最后一个初始化的
    InitTask(&t, TaskB);
    InitTask(&p, TaskA);   
}

void LaunchTask()
{
    gCTaskAddr = &p;  //第一个任务是TaskB的时候可以实现切换 差距在初始化顺序
    
    //启动任务
    RunTask(gCTaskAddr);
}

void Schedule()
{
   gCTaskAddr = (gCTaskAddr == &p) ? &t : &p;
 
   //SetPrintPos(0, 15);  
   //PrintIntHex(gCTaskAddr);
   
   gTSS.ss0 = GDT_DATA32_FLAT_SELECTOR;    //设置0特权级的栈
   gTSS.esp0 = (uint)&gCTaskAddr->rv + sizeof(gCTaskAddr->rv); //esp指针指向 Task结构体RegValue的末尾 目的是在调用中断时自动完成ss esp eflags cs的压栈 
   
   SetDescValue(AddrOff(gGdtInfo.entry, GDT_TASK_LDT_INDEX), (uint)&gCTaskAddr->ldt, sizeof(gCTaskAddr->ldt)-1, DA_LDT + DA_DPL0);  //重新设置gdt内的ldt 切换到对应任务的ldt
   
   LoadTask(gCTaskAddr);   
}



