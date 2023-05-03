
%include "common.asm"

; 本文件定义 给外部使用
global _start
global AppModInit

extern AppMain
extern GetAppToRun
extern GetAppNum

[section .text]
[bits 32]
_start:
AppModInit: ; 0xf000
     ;遵循c语言调用约定
     push ebp
     mov ebp, esp

     ;将函数入口放入共享内存区 
     mov dword [GetAPPNumEntry], GetAppNum   
     mov dword [GetAppToRunEntry], GetAppToRun

     call AppMain
     
     leave
     
     jmp $


     
