
%include "blfunc.asm"
%include "common.asm"

org BaseOfBoot 

BaseOfStack   equ   BaseOfBoot
Loader db  "LOADER     "           ;加载的文件名
LdLen equ ($-Loader)              ;文件名长度


BLMain:
    ;初始化各个段寄存器  
    mov ax, cs  ;cs为代码段起始 段地址
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov sp, SPInitValue ;设置栈地址

    push word Buffer
    push word BaseOfLoader / 0x10
    push word BaseOfLoader
    push word LdLen
    push word Loader

    call LoadTarget
    
    cmp dx, 0
    jz output
    jmp BaseOfLoader    
	
	
output:
    mov ax, cs
    mov es, ax
    mov bp, ErrStr
    mov cx, ErrLen
    xor dx, dx
    mov ax, 0x1301
    mov bx, 0x0007
    int 0x10    ;调用bios中断打印字符串
    jmp $

ErrStr db "NoLD"
ErrLen equ ($-ErrStr) ;$代表当前行地址 结果是得到MsgStr的长度
Buffer:
    times 510-($-$$) db 0x00 ;$为当前行地址 $$为代码起始行地址 作用是填充数据到512字节
    db 0x55, 0xaa ;结尾填上主引导程序标识
