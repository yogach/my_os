
%include "common.asm"

global _start

extern gGdtInfo
extern gIdtInfo
extern KMain
extern RunTask
extern InitInterrupt
extern EnableTimer
extern SendEOI
extern ClearScreen

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
                    
     leave  ;关闭栈帧
     
     ret     
