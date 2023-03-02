
BaseOfBoot  equ  0x7c00 ;起始地址

org BaseOfBoot 

%include "blfunc.asm"

interface:
    BaseOfStack   equ   BaseOfBoot
    BaseOfTarget  equ   0x9000      ;加载地址
    Target db  "LOADER     "        ;加载的文件名
    TarLen equ ($-Target)           ;文件名长度


BLMain:
    ;初始化各个段寄存器  
    mov ax, cs  ;cs为代码段起始 段地址
    mov ss, ax
	mov ds, ax
	mov es, ax
	mov sp, SPInitValue ;设置栈地址
	
	call LoadTarget
	
	cmp dx, 0
	jz output
	jmp BaseOfTarget    
	
	
output:
	mov bp, ErrStr
	mov cx, ErrLen
	call print
	
last:
    hlt     ;休眠一段时间
    jmp last

ErrStr db "No LOADER ..."
ErrLen equ ($-ErrStr) ;$代表当前行地址 结果是得到MsgStr的长度
Buffer:
    times 510-($-$$) db 0x00 ;$为当前行地址 $$为代码起始行地址 作用是填充数据到512字节
    db 0x55, 0xaa ;结尾填上主引导程序标识
