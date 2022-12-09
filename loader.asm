%include "inc.asm"

org 0x9000

jmp ENTRY_SEGMENT

[section .gdt]  
; GDT definition
;                               段基址，            段界限，          段属性
GDT_ENTRY       : Descriptor      0,                  0,                0
CODE32_DESC     : Descriptor      0,             Code32Seglen - 1,    DA_C + DA_32
VIDEO_DESC      : Descriptor   0xB8000,          0xBFFFF - 0xB8000,   DA_DRWA + DA_32
DATA32_DESC     : Descriptor      0,             Data32SegLen - 1,    DA_DR + DA_32
STACK32_DESC    : Descriptor      0,             TopOfStack32,        DA_DRW + DA_32
CODE16_DESC     : Descriptor      0,               0xFFFF,            DA_C
UPDATA_DESC     : Descriptor      0,               0xFFFF,            DA_DRW
TASK_A_LDT_DESC : Descriptor      0,             TaskALdtLen - 1,     DA_LDT
FUNCTION_DESC   : Descriptor      0,            FunctionSegLen - 1,   DA_C + DA_32
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
Code16Selector   equ (0x0005 << 3) + SA_TIG + SA_RPL0
UpdateSelector   equ (0x0006 << 3) + SA_TIG + SA_RPL0
TaskALdtSelector equ (0x0007 << 3) + SA_TIG + SA_RPL0
FunctionSelector equ (0x0008 << 3) + SA_TIG + SA_RPL0

; end of [section .gdt]		  

TopOfStack16   equ 0x7c00 

;32位数据段
[section .dat]
[bits 32]
DATA32_SEGMENT:
	DTOS               db  "D.T.OS!", 0
	DTOS_OFFSET        equ DTOS - $$   ;得到DTOS数据段在DATA32_SEGMENT的偏移位置
	HELLO_WORLD        db  "Hello World!", 0
	HELLO_WORLD_OFFSET equ HELLO_WORLD - $$

Data32SegLen equ $ - DATA32_SEGMENT

;16位 实模式代码段
[section .s16]
[bits 16] 
ENTRY_SEGMENT:
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, TopOfStack16
	
	mov [BACK_TO_REAL_MODE + 3], ax ; 修改对应指令的地址 将cs替换掉对应的0跳转地址
	
	; initialize GDT for 32bit code segment
	mov esi, CODE32_SEGMENT
	mov edi, CODE32_DESC
	
	call InitDescItem
	
	mov esi, DATA32_SEGMENT
	mov edi, DATA32_DESC
	call InitDescItem
	
	;定义32位栈段的段基地址
	mov esi, STACK32_SEGMENT
	mov edi, STACK32_DESC
	call InitDescItem
	
	;定义16位保护模式段 段基地址
	mov esi, CODE16_SEGMENT
	mov edi, CODE16_DESC

	call InitDescItem

    ;设置局部段描述符的段基址
    mov esi, TASK_A_LDT_ENTRY
    mov edi, TASK_A_LDT_DESC
    
    call InitDescItem
	
	;设置局部段的段基址
	mov esi, TASK_A_CODE32_SEGMENT
	mov edi, TASK_A_CODE32_DESC

	call InitDescItem

	mov esi, TASK_A_DATA32_SEGMENT
	mov edi, TASK_A_DATA32_DESC

	call InitDescItem	
	
	mov esi, TASK_A_STACK32_SEGMENT
	mov edi, TASK_A_STACK32_DESC

	call InitDescItem

	mov esi, FUNCTION_SEGMENT
	mov edi, FUNCTION_DESC

	call InitDescItem	
	
	; initialize GDT pointer struct
	mov eax, 0
	mov ax, ds
	shl eax, 4
	add eax, GDT_ENTRY
	mov dword [GdtPtr + 2], eax  ;设置GDT入口物理地址
	
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
	 
BACK_ENTRY_SEGMENT:
    ;重新设置寄存器
    mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, TopOfStack16
	
	;关闭a20总线
	in al, 0x92
	and al, 11111101b
	out 0x92, al
	
	;打开中断
	sti
	
	mov bp, HELLO_WORLD
	mov cx, 12
	mov dx, 0
	mov ax, 0x1301
	mov bx, 0x0007
	int 0x10
	
	jmp $

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
	
[section .s16]
[bits 16]
CODE16_SEGMENT:
    ;使用选择子来刷新高速缓冲寄存器
    mov ax, UpdateSelector
    mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	
	;退出保护模式
	mov eax, cr0
	and al, 11111110b
	mov cr0, eax
    	 

BACK_TO_REAL_MODE:
    jmp 0 : BACK_ENTRY_SEGMENT  ;跳转到实模式 因为进入此段时还是保护模式 所以这里的跳转地址如果超过了段界限 cpu会报异常
    

Code16SegLen equ  $ - CODE16_SEGMENT

[section .func]
[bits 32]
FUNCTION_SEGMENT:
; ds:ebp  --> string address
; bx      --> attribute 
; dx      --> dh : row, dl : col
PrintStringFunc:
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
   
   retf

PrintString equ PrintStringFunc - $$

FunctionSegLen  equ  $ - FUNCTION_SEGMENT
	 
; 32为保护模式代码段	 
[section .s32]
[bits 32]	 ;定义为32位编译
CODE32_SEGMENT:
	mov ax, VideoSelector
	mov gs, ax            ;显存段选择子
	
	;设置栈 段地址
	mov ax, StackSelector
	mov ss, ax
	
	;设置栈顶指针
	mov eax, TopOfStack32
	mov esp, eax
	
	mov ax, Data32Selector
	mov ds, ax
	
	mov ebp, DTOS_OFFSET
	mov bx, 0x0c
	mov dh, 12
	mov dl, 33
	
	call FunctionSelector : PrintString
	
	mov ebp, HELLO_WORLD_OFFSET
	mov bx, 0x0c
	mov dh, 13
	mov dl, 33
	
	call FunctionSelector : PrintString
	
	;加载局部段描述符
	mov ax, TaskALdtSelector
	
	lldt ax
	
	jmp TaskACode32Selector : 0
	
	;jmp Code16Selector : 0	  ;保护模式回实模式入口
   
Code32Seglen equ  $ -  CODE32_SEGMENT

;定义保护模式栈段
[section .gs]
[bits 32]
STACK32_SEGMENT:
    times 1024 * 4 db 0
	
Stack32SegLen equ $ - STACK32_SEGMENT
TopOfStack32  equ Stack32SegLen - 1

; ==========================================
;
;            Task A Code Segment 
;
; ==========================================

[section .task-a-ldt]
; Task A LDT definition
;                                         段地址，         段界限，              段属性
TASK_A_LDT_ENTRY:
TASK_A_CODE32_DESC    :   Descriptor        0,          TaskACode32SegLen - 1,   DA_C + DA_32
TASK_A_DATA32_DESC    :   Descriptor        0,          TaskAData32SegLen - 1,   DA_DR + DA_32
TASK_A_STACK32_DESC   :   Descriptor        0,          TaskAStack32SegLen - 1,  DA_DRW + DA_32
	 
TaskALdtLen  equ  $ - TASK_A_LDT_ENTRY  ;计算得到局部段描述符表的长度	 

; Task A LDT Selector 
TaskACode32Selector   equ   (0x0000 << 3) + SA_TIL + SA_RPL0
TaskAData32Selector   equ   (0x0001 << 3) + SA_TIL + SA_RPL0
TaskAStack32Selector  equ   (0x0002 << 3) + SA_TIL + SA_RPL0

[section .task-a-dat]
[bits 32]
TASK_A_DATA32_SEGMENT:
	TASK_A_STRING        db  "this is task a", 0
	TASK_A_STRING_OFFSET equ TASK_A_STRING - $$
	
TaskAData32SegLen equ $ - TASK_A_DATA32_SEGMENT

[section .task-a-gs]
[bits 32]
TASK_A_STACK32_SEGMENT:
	times 1024 db 0
	
TaskAStack32SegLen equ  $ -  TASK_A_STACK32_SEGMENT
TaskATopOfStack32  equ  TaskAStack32SegLen - 1

[section .task-a-s32]
[bits 32]
TASK_A_CODE32_SEGMENT:
    ;重新设置一些段寄存器
    mov ax, VideoSelector
    mov gs, ax
	
	mov ax, TaskAStack32Selector
	mov ss, ax
	
	mov eax, TaskATopOfStack32
	mov esp, eax
	
	mov ax, TaskAData32Selector
	mov ds, ax
	
	mov ebp, TASK_A_STRING_OFFSET
	mov bx, 0x0c
	mov dh, 14
	mov dl, 29
	
	call FunctionSelector : PrintString 
	
	jmp Code16Selector : 0 

  	
TaskACode32SegLen   equ $ - TASK_A_CODE32_SEGMENT	

	