
%include "common.asm"

; 本文件定义 给外部使用
global _start
global TimerHandlerEntry
global SysCallHandlerEntry
global PageFaultHandlerEntry
global SegmentFaultHandlerEntry
global KeyboardHandlerEntry


global ReadPort
global WritePort

; 引用外部的变量
extern gGdtInfo
extern gIdtInfo
extern gCTaskAddr
extern KMain
extern RunTask
extern LoadTask
extern InitInterrupt
extern SendEOI
extern ClearScreen


extern TimerHandler
extern SysCallHandler
extern PageFaultHandler
extern SegmentFaultHandler
extern KeyboardHandler

;定义宏 0 代表不需要参数
%macro BeginISR  0
   cli      ;关闭中断
    
   sub esp, 4  ;跳过RegValue的raddr
   
   ; 通用寄存器压栈 保存现场
   pushad      
   
   push ds
   push es
   push fs
   push gs
   
   mov si, ss   
   mov ds, si
   mov es, si
   
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

;定义宏 0 代表不需要参数
%macro BeginFSR  0
   cli         ;关闭中断

   ; 异常中RegValue的raddr 实际上保存的是异常发生时的error code
   ; sub esp, 4  
   
   ; 通用寄存器压栈 保存现场
   pushad      
   
   push ds
   push es
   push fs
   push gs
   
   mov si, ss   
   mov ds, si
   mov es, si
   
   mov esp, BaseOfLoader   ;完成保存现场之后重新设置内核栈
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
     
     mov eax, dword [SendEOIEntry]
     mov dword [SendEOI], eax
     
     mov eax, dword [LoadTaskEntry]
     mov dword [LoadTask], eax     
                    
     leave  ;关闭栈帧
     
     ret
;
; byte ReadPort(ushort port)
;
ReadPort:
     push ebp
     mov ebp, esp
     
     xor eax, eax  ; 将eax本身置为0

     mov dx, [esp + 8] ;取得c语言调用传入的参数 实际就是传入的端口
     in al, dx         ; 读取对应的端口值

     nop 
     nop
     nop

     leave

     ret

;
; void WritePort(ushort port, byte value)
;
WritePort:
     push ebp 
     mov ebp, esp
     
     xor eax, eax

     mov dx, [esp + 8]
     mov al, [esp + 12]
     out dx, al

     nop 
     nop
     nop

     leave

     ret    

;
;
TimerHandlerEntry:
BeginISR
    call TimerHandler
EndISR

;
;
KeyboardHandlerEntry:
BeginISR
    call KeyboardHandler
EndISR
;
;
SysCallHandlerEntry:
BeginISR
    push edx   ;将参数压入栈 参数遵循从右到左的顺序入栈
    push ecx
    push ebx 
    push eax
    call SysCallHandler
    pop eax
    pop ebx
    pop ecx
    pop edx
EndISR

;
;
PageFaultHandlerEntry:
BeginFSR
    call PageFaultHandler
EndISR

;
;
SegmentFaultHandlerEntry:
BeginFSR
    call SegmentFaultHandler
EndISR


     
