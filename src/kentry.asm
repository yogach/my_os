
%include "common.asm"

; 本文件定义 给外部使用
global _start
global TimerHandlerEntry
global SysCallHandlerEntry

; 引用外部的变量
extern gGdtInfo
extern gIdtInfo
extern gCTaskAddr
extern KMain
extern RunTask
extern LoadTask
extern InitInterrupt
extern EnableTimer
extern SendEOI
extern ClearScreen
extern TimerHandler
extern SysCallHandler

;定义宏 0 代表不需要参数
%macro BeginISR  0
   sub esp, 4  ;跳过RegValue的raddr
   
   ; 通用寄存器压栈 保存现场
   pushad      
   
   push ds
   push es
   push fs
   push gs
   
   mov dx, ss   
   mov ds, dx
   mov es, dx
   
   mov esp, BaseOfLoader   ;完成保存现场之后重新设置内核栈
%endmacro

%macro EndISR  0
   mov esp, [gCTaskAddr]   ; 将esp指向Task结构的起始位置
   
   ;寄存器出栈 恢复现场
   pop gs
   pop fs
   pop es
   pop ds
   
   popad
   
   add esp, 4 ;跳过RegValue的raddr
   
   iret
   
%endmacro

[section .text]
[bits 32]
_start:
     mov ebp, 0
     
     call InitGlobal
     call ClearScreen
     call KMain           ;跳转执行c函数入口
     
     jmp $

;
;
InitGlobal:
     push ebp
     mov ebp, esp
     
     ;将共享内存中的值取出来
     mov eax, dword [GdtEntry]
     mov [gGdtInfo], eax
     mov eax, dword [GdtSize]
     mov [gGdtInfo + 4], eax
     
     mov eax, dword [IdtEntry]
     mov [gIdtInfo], eax
     mov eax, dword [IdtSize]
     mov [gIdtInfo + 4], eax     
     
     mov eax, dword [RunTaskEntry]
     mov dword [RunTask], eax  

     mov eax, dword [InitInterruptEntry]
     mov dword [InitInterrupt], eax

     mov eax, dword [EnableTimerEntry]
     mov dword [EnableTimer], eax
     
     mov eax, dword [SendEOIEntry]
     mov dword [SendEOI], eax
     
     mov eax, dword [LoadTaskEntry]
     mov dword [LoadTask], eax     
                    
     leave  ;关闭栈帧
     
     ret

;
;
TimerHandlerEntry:
BeginISR
    call TimerHandler
EndISR

SysCallHandlerEntry:
BeginISR
    push ax   ;后续继续看一下 汇编调用c语言时 栈上是怎么设置的
    call SysCallHandler
    pop ax
EndISR
     
