%include "inc.asm"

org 0x9000

jmp CODE16_SEGMENT

[section .gdt]  
; GDT definition
;                               段基址，            段界限，          段属性
GDT_ENTRY       : Descriptor      0,                  0,                0
CODE32_DESC     : Descriptor      0,             Code32Seglen - 1,    DA_C + DA_32
; GDT end

GdtLen    equ $ - GDT_ENTRY ; GDT长度

;定义Gdt描述 
GdtPtr:
          dw GdtLen - 1  ;GDT长度
		  dd 0           ;GDT入口物理地址
		  
; GDT Selector
;定义选择子 描述符索引值为1
Code32Selector   equ (0x0001 << 3) + SA_TIG + SA_RPL0

; end of [section .gdt]		  

[section .s16]
[bits 16] ;定义为16位编译
CODE16_SEGMENT:
     mov ax, cs
	 mov ds, ax
	 mov es, ax
	 mov ss, ax
	 mov sp, 0x7c00
	 
	 
	 ; initialize GDT for 32bit code segment
	 mov eax, 0
	 mov ax, cs
	 shl eax, 4  ;左移4位
	 add eax, CODE32_SEGMENT ;得到CODE32_SEGMENT的物理地址
	 
	 ;设置CODE32_DESC中的保护段的段基址
	 mov word [CODE32_DESC + 2], ax
	 shr eax, 16
	 mov byte [CODE32_DESC + 4], al
	 mov byte [CODE32_DESC + 7], ah
	 
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
	 jmp dword Code32Selector : 0
	 
[section .s32]
[bits 32]	 ;定义为32位编译
CODE32_SEGMENT:
	 mov eax, 0
	 jmp CODE32_SEGMENT
	 
Code32Seglen equ  $ -  CODE32_SEGMENT
	 