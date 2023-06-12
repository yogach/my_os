
#include "syscall.h"

void Exit()
{
  int i;

	asm volatile(
	    "movl $0, %eax \n"   //type
	    "int  $0x80     \n"
	);	
}

uint CreateMutex()
{
	volatile uint ret = 0;

	PrintString("&ret = ");
	PrintIntHex(&ret);
	
	asm volatile(
	    "movl $1, %%eax \n"   //type  $1 代表立即数
	    "movl $0, %%ebx \n"   //cmd 
	    "movl %0, %%ecx \n"   //param1  %0 代表传入变量
	    "int $0x80     \n"
	    : 
	    : "r"(&ret)                   //"r" 定义为一个输入操作数 并且使用 &ret 取得了ret变量的地址
			: "eax", "ebx", "ecx", "edx"  //告诉编译器哪些寄存器在内联汇编代码块中被修改或使用，并防止编译器在代码优化时将这些寄存器中的值保留下来，从而避免出现未定义行为
	);	

	return ret;
}

void EnterCritical(uint mutex)
{
	asm volatile(
	    "movl $1, %%eax \n"   //type  $1 代表立即数
	    "movl $1, %%ebx \n"   //cmd 
	    "movl %0, %%ecx \n"   //param1  %0 代表传入变量
	    "int $0x80     \n"
	    : 
	    : "r"(mutex)                   
			: "eax", "ebx", "ecx", "edx"  
	);	 
}

void ExitCritical(uint mutex)
{
	asm volatile(
	    "movl $1, %%eax \n"   //type  $1 代表立即数
	    "movl $2, %%ebx \n"   //cmd 
	    "movl %0, %%ecx \n"   //param1  %0 代表传入变量
	    "int $0x80     \n"
	    : 
	    : "r"(mutex)                   
			: "eax", "ebx", "ecx", "edx"  
	);	
}

void DestroyMutex(uint mutex)
{
	asm volatile(
	    "movl $1, %%eax \n"   //type  $1 代表立即数
	    "movl $3, %%ebx \n"   //cmd 
	    "movl %0, %%ecx \n"   //param1  %0 代表传入变量
	    "int $0x80     \n"
	    : 
	    : "r"(mutex)                   
			: "eax", "ebx", "ecx", "edx"  
	);	
}

