#ifndef TASK_H
#define TASK_H

#include "kernel.h"
#include "queue.h"

//任务的寄存器列表
typedef struct {
    uint gs;
    uint fs;
    uint es;
    uint ds;
    uint edi;
    uint esi;
    uint ebp;
    uint kesp;
    uint ebx;
    uint edx;
    uint ecx;
    uint eax;
    uint raddr;
    uint eip;
    uint cs;
    uint eflags;
    uint esp;
    uint ss;
} RegValue;

typedef struct
{
    uint   previous;
    uint   esp0;
    uint   ss0;
    uint   unused[22];
    ushort reserved;
    ushort iomb;
} TSS;

//Task 用于描述一个任务的所有信息
typedef struct
{
	  //需注意这几个变量的顺序不要改 改变后导致实际任务运行失败 在RunTask里按固定值加载
    RegValue   rv;           //寄存器列表
    Descriptor ldt[3];       //局部段描述符表
    //TSS        tss;          //任务段 主要功能是用于记录各特权级的栈信息    
    ushort     ldtSelector;  //局部段描述符表选择子
    ushort     tssSelector;  //任务段选择子

		//新增变量在此开始新增
    void  (*tmain)();
    uint       id;
    char       name[8]; 
    byte       stack[512];   //任务使用的栈
} Task;

typedef struct
{
    QueueNode head;
    Task task;
} TaskNode;

extern void (* const RunTask)(volatile Task* pt);
extern void (* const LoadTask)(volatile Task* pt);

void TaskModInit();
void LaunchTask();
void Schedule();
void KillTask();


#endif
