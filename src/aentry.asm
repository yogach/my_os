
%include "common.asm"

; 本文件定义 给外部使用
global _start
global AppModInit

extern AppMain
extern GetAppToRun
extern GetAppNum
extern MemModInit

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

     ;c语言调用约定 参数从右到左入栈
     push HeapSize
     push AppHeapBase

     call MemModInit  ;调用内存管理模块初始化

     add esp, 8  ;清理栈帧

     call AppMain
     
     leave
     
     ret


     
