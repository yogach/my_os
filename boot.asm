org 0x7c00 ;起始地址

start:
    ;初始化各个段寄存器  
    mov ax, cs  ;cs为代码段起始 段地址
mov ss, ax
mov ds, ax
mov es, ax

mov si, msg ;取得msg标号的地址

print:
    mov al, [si]
    add si, 1
    cmp al, 0x00 ;判断是否到末尾
    je last
    
	;调用biso中断进行打印
    mov ah, 0x0e
    mov bx, 0x0f
    int 0x10
    jmp print

last:
    hlt     ;休眠一段时间
    jmp last

msg:
    db 0x0a, 0x0a
    db "Hello, DTOS!"
    db 0x0a, 0x0a
times 510-($-$$) db 0x00 ;$为当前行地址 $$为代码起始行地址 作用是填充数据到512字节
db 0x55, 0xaa ;结尾填上主引导程序标识