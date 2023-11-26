#include "screen.h"
#include "task.h"
#include "interrupt.h"
#include "memory.h"
#include "mutex.h"
#include "keyboard.h"
#include "hdraw.h"

void KMain()
{
    void (*AppModInit)() = (void*)BaseOfApp;
	byte* pn = (byte*)0x475; //平坦内存模式 虚拟地址和实际地址是一样的

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

    //打印扫描到的硬盘数
    PrintString("Number of Hard Disk: ");
    PrintIntDec(*pn);
    PrintChar('\n');

	MemModInit(KernelHeapBase, HeapSize);

	n = HDRawSectors();
    //打印扫描到的硬盘数
    PrintString("Number of Sectors: ");
    PrintIntDec(n);
	PrintString(" bytes ");
    PrintChar('\n');

	pn = Malloc(SECT_SIZE);

	pn[1] = 0xCD;
	pn[128] = 0xEF;
	pn[511] = 0xAB;

	n = HDRawWrite(2, pn);

    PrintString("n= ");
    PrintIntDec(n);
    PrintChar('\n');

	pn[1] = 0;
	pn[128] = 0;
	pn[511] = 0;

	n = HDRawRead(2, pn);

    PrintString("pn[1]= ");
    PrintIntDec(pn[1]);
    PrintChar('\n');

    PrintString("pn[128]= ");
    PrintIntDec(pn[128]);
    PrintChar('\n');

    PrintString("pn[511]= ");
    PrintIntDec(pn[511]);
    PrintChar('\n');

	KeyboardModInit();

	MutexModInit();

	//AppModInit();
    
    TaskModInit();
    
    IntModInit(); //初始化中断

	ConfigPageTable();

	while(1);
    
    LaunchTask(); //开始执行任务
}
