
#ifndef KERNEL_H
#define KERNEL_H

#include "type.h"
#include "const.h"

typedef struct{
    ushort limit1;
    ushort base1;
    byte base2;
    byte attr1;
    byte attr2_limit2;
    byte base3;    
}Descriptor;

typedef struct{
    ushort offset1;
    ushort selector;
    byte dcount;
    byte attr;
    ushort offset2;    
}Gate;


typedef struct {
    Descriptor * const entry;
    const int          size;
} GdtInfo;

typedef struct {
    Gate * const entry;
    const int          size;
} IdtInfo;

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
    RegValue   rv;           //寄存器列表
    Descriptor ldt[3];       //局部段描述符表
    TSS        tss;          //任务段 主要功能是用于记录各特权级的栈信息
    ushort     ldtSelector;  //局部段描述符表选择子
    ushort     tssSelector;  //任务段选择子
    uint       id;
    char       name[8]; 
    byte       stack[512];   //任务使用的栈
} Task;

int SetDescValue(Descriptor* pDesc, uint base, uint limit, ushort attr);
int GetDescValue(Descriptor* pDesc, uint* pBase, uint* pLimit, ushort* pAttr);
int SetIntHandler(Gate* pGate, uint ifunc);
int GetIntHandler(Gate* pGate, uint* pIFunc);


#endif
