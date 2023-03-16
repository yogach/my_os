
%include "blfunc.asm"
%include "common.asm"

org BaseOfLoader

interface:
    BaseOfStack   equ   BaseOfLoader
    BaseOfTarget  equ   BaseOfKernel      ;加载地址
    Target db  "KERNEL     "              ;加载的文件名
    TarLen equ ($-Target)                 ;文件名长度

[section .gdt]
; GDT definition
;                                      Base,         Limit,         Attribute
GDT_ENTRY            :     Descriptor    0,            0,           0
CODE32_DESC          :     Descriptor    0,     Code32SegLen - 1,   DA_C + DA_32 + DA_DPL0
VIDEO_DESC           :     Descriptor    0xB8000,    0x07FFF,       DA_DRWA + DA_32 + DA_DPL0
CODE32_FLAT_DESC     :     Descriptor    0,          0xFFFFF,       DA_C + DA_32 + DA_DPL0
DATA32_FLAT_DESC     :     Descriptor    0,          0xFFFFF,       DA_DRW + DA_32 + DA_DPL0
TASK_LDT_DESC        :     Descriptor    0,          0,             0
TASK_TSS_DESC        :     Descriptor    0,          0,             0


; GDT end

GdtLen    equ   $ - GDT_ENTRY

GdtPtr:
          dw   GdtLen - 1
          dd   0
          
          
; GDT Selector
Code32Selector        equ (0x0001 << 3) + SA_TIG + SA_RPL0
VideoSelector         equ (0x0002 << 3) + SA_TIG + SA_RPL0
Code32FlatSelector    equ (0x0003 << 3) + SA_TIG + SA_RPL0
Data32FlatSelector    equ (0x0004 << 3) + SA_TIG + SA_RPL0


; end of [section .gdt]

[section .s16]
[bits 16]
BLMain:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, SPInitValue
    
    ; initialize GDT for 32 bits code segment
    mov esi, CODE32_SEGMENT
    mov edi, CODE32_DESC
    
    call InitDescItem
     
    ; initialize GDT pointer struct
    mov eax, 0
    mov ax, ds
    shl eax, 4
    add eax, GDT_ENTRY
    mov dword [GdtPtr + 2], eax  ;计算gdt的地址
    
    call LoadTarget
    
    cmp dx, 0
    jz output
    
    call StoreGlobal

    ; 1. load GDT
    lgdt [GdtPtr]
    
    ; 2. close interrupt
    ;    set IOPL to 3
    cli 
    
    pushf
    pop eax
    
    or eax, 0x3000
    
    push eax
    popf
    
    ; 3. open A20
    in al, 0x92
    or al, 00000010b
    out 0x92, al
    
    ; 4. enter protect mode
    mov eax, cr0
    or eax, 0x01
    mov cr0, eax
    
    ; 5. jump to 32 bits code
    jmp dword Code32Selector : 0

output:	
    mov bp, ErrStr
    mov cx, ErrLen
	call print
	
	jmp $

; esi    --> code segment label
; edi    --> descriptor label
InitDescItem:
    push eax

    mov eax, 0
    mov ax, cs
    shl eax, 4
    add eax, esi
    mov word [edi + 2], ax
    shr eax, 16
    mov byte [edi + 4], al
    mov byte [edi + 7], ah
    
    pop eax
    
    ret
 
;
; 填充共享内存区数据 
StoreGlobal:
    mov dword [RunTaskEntry], RunTask

    mov eax, dword [GdtPtr + 2]
    mov dword [GdtEntry], eax
    
    mov dword [GdtSize], GdtLen / 8
    
    ret

;代码段的意义？
[section .gfunc]
[bits 32]
;
; parameter   ===> Task* pt
RunTask:
    push ebp 
    mov ebp, esp
    
    ;移动到参数的起始位置 原先的esp保存的是old ebp [esp + 4]是返回地址
    ;c函数调用在传递参数的时候 高地址数据也是保存在栈的高地址上的
    mov esp, [esp + 8] ; mov esp , &(pt->rv.gs) 
    
    lldt word [esp + 200] ; 加载局部段描述符表
    ltr word [esp + 202]  ; 加载tss
    
    pop gs
    pop fs
    pop es
    pop ds
    
    popad      ; pop edi, esi, ebp, esp, ebx, edx, ecx, eax
    
    add esp, 4 ; mov esp, &(pt->rv.eip)
    
    iret       ; 调用iret的同时 会将 eip cs elags esp ss 寄存器出栈 用于恢复现场
    
[section .s32]
[bits 32]
CODE32_SEGMENT:
    mov ax, VideoSelector
    mov gs, ax
    
    mov ax, Data32FlatSelector
    mov ds, ax
    mov es, ax
    mov fs, ax
    
    mov ax, Data32FlatSelector
    mov ss, ax
    mov esp, BaseOfLoader
    
    ;跳转到内核入口执行
    jmp dword Code32FlatSelector : BaseOfTarget

Code32SegLen    equ    $ - CODE32_SEGMENT

ErrStr db "No LOADER ..."
ErrLen equ ($-ErrStr) ;$代表当前行地址 结果是得到MsgStr的长度
Buffer db 0
