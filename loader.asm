%include "inc.asm"

org 0x9000

jmp CODE16_SEGMENT

[section .gdt]  
; GDT definition
;                               段基址，            段界限，          段属性
GDT_ENTRY       : Descriptor      0,                  0,                0
CODE32_DESC     : Descriptor      0,             Code32Seglen - 1,    DA_C + DA_32
VIDEO_DESC      : Descriptor   0xB8000,          0xBFFFF - 0xB8000,   DA_DRWA + DA_32
DATA32_DESC     : Descriptor      0,             Data32SegLen - 1,    DA_DR + DA_32
STACK_DESC      : Descriptor      0,             TopOfStackInit,      DA_DRW + DA_32
; GDT end

GdtLen    equ $ - GDT_ENTRY ; GDT长度

;定义Gdt描述 
GdtPtr:
          dw GdtLen - 1  ;GDT长度
		  dd 0           ;GDT入口物理地址
		  
; GDT Selector
;定义选择子 索引值需要和GDT表中的位置对应
Code32Selector   equ (0x0001 << 3) + SA_TIG + SA_RPL0
VideoSelector    equ (0x0002 << 3) + SA_TIG + SA_RPL0
Data32Selector   equ (0x0003 << 3) + SA_TIG + SA_RPL0
StackSelector    equ (0x0004 << 3) + SA_TIG + SA_RPL0

; end of [section .gdt]		  

TopOfStackInit   equ 0x7c00 

[section .dat]
[bits 32]
DATA32_SEGMENT:
	DTOS               db  "D.T.OS!", 0
	DTOS_OFFSET        equ DTOS - $$   ;得到DTOS数据段在DATA32_SEGMENT的偏移位置
	HELLO_WROLD        db  "Hello World!", 0
	HELLO_WROLD_OFFSET equ HELLO_WROLD - $$

Data32SegLen equ $ - DATA32_SEGMENT

[section .s16]
[bits 16] ;定义为16位编译
CODE16_SEGMENT:
     mov ax, cs
	 mov ds, ax
	 mov es, ax
	 mov ss, ax
	 mov sp, TopOfStackInit
	 
	 
	 ; initialize GDT for 32bit code segment
	 mov esi, CODE32_SEGMENT
	 mov edi, CODE32_DESC
	 
	 call InitDescItem
	 
	 mov esi, DATA32_SEGMENT
	 mov edi, DATA32_DESC
	 call InitDescItem
	 
	 ; initialize GDT pointer struct
	 mov eax, 0
	 mov ax, ds
	 shl eax, 4
	 add eax, GDT_ENTRY
	 mov dword [GdtPtr + 2], eax  ;GDT入口物理地址
	 
	 ; 1.load GDT
	 lgdt [GdtPtr]
	 
	 ; 2. close interrupt
	 cli
	 
	 ; 3. open A20 读入0x92寄存器数据 修改后重新写入
	 in al, 0x92      
	 or al, 00000010b
	 out 0x92, al
	 
	 ; 4. enter protect mode
	 mov eax, cr0
	 or eax, 0x01
	 mov cr0, eax
	 
	 ; 5. jmp to 32 bits code 使用选择子进行跳转 进入保护模式
	 jmp dword Code32Selector : 0 ;跳转到 CODE32_SEGMENT : 0开始的地方

; esi --> code segment label
; edi --> descriptor label
InitDescItem:
     push eax
	 
	 mov eax, 0
	 mov ax, cs
	 shl eax, 4
	 add eax, esi  ; eax 先左移4位 + code segment label的地址 = code segment label的物理地址
	 
	 ;设置code segment label的段基址
	 mov word [edi + 2], ax
	 shr eax, 16
	 mov byte [edi + 4], al
	 mov byte [edi + 7], ah	 
	 
	 pop eax
	 ret
	 
[section .s32]
[bits 32]	 ;定义为32位编译
CODE32_SEGMENT:
	 mov ax, VideoSelector
	 mov gs, ax            ;显存段选择子
	 
	 mov ax, StackSelector
	 mov ss, ax
	 
	 mov ax, Data32Selector
	 mov ds, ax
	 
	 mov ebp, DTOS_OFFSET
	 mov bx, 0x0c
	 mov dh, 12
	 mov dl, 33
	 
	 call PrintString
	 
	 mov ebp, HELLO_WROLD_OFFSET
	 mov bx, 0x0c
	 mov dh, 13
	 mov dl, 33
	 
	 call PrintString
	 
	 
	 jmp $	 

; ds:ebp  --> string address
; bx      --> attribute 
; dx      --> dh : row, dl : col
PrintString:
	  push ebp
	  push eax
	  push edi
	  push cx
	  push dx

print:
      mov cl, [ds:ebp]
      cmp cl, 0       
	  je end     ;cl为0 代表字符串打印结束
	  
	  ; edi = (80 * dh + dl) * 2
	  mov eax, 80
	  mul dh
	  add al, dl
	  shl eax, 1 ;左移一位等于*2
	  mov edi, eax 
	  
	  mov ah, bl ;字符的属性
	  mov al, cl ;要显示的字符
	  mov [gs:edi], ax
	  inc ebp
	  inc dl
	  jmp print
	  

end:
      pop dx
      pop cx
      pop edi
      pop eax
      pop ebp

      ret	  
	  
	 
Code32Seglen equ  $ -  CODE32_SEGMENT
	 