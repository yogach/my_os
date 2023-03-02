
; Segment Attribute
DA_32    equ    0x4000  ; 保护模式下32位段
DA_DR    equ    0x90    ; 只读数据段
DA_DRW   equ    0x92    ; 可读写数据段
DA_DRWA  equ    0x93    ; 已访问可读写数据段
DA_C     equ    0x98    ; 只执行代码段
DA_CR    equ    0x9A    ; 可执行可读代码段 
DA_CCO   equ    0x9C    ; 只执行一致代码段
DA_CCOR  equ    0x9E    ; 可执行可读一致代码段

; Segment Privilege
DA_DPL0		equ	  0x00    ; DPL = 0
DA_DPL1		equ	  0x20    ; DPL = 1
DA_DPL2		equ	  0x40    ; DPL = 2
DA_DPL3		equ	  0x60    ; DPL = 3

; Special Attribute
DA_LDT       equ    0x82
DA_TaskGate  equ    0x85  ; 任务门类型值
DA_386TSS    equ    0x89  ; 可用 386 任务状态段类型值
DA_386CGate  equ    0x8c  ; 386 调用门类型值
DA_386IGate  equ    0x8e  ; 386 中断门类型值
DA_386TGate  equ    0x8f  ; 386 陷阱门类型值

; Selector Attribute
SA_RPL0    equ    0
SA_RPL1    equ    1
SA_RPL2    equ    2
SA_RPL3    equ    3

SA_TIG    equ    0
SA_TIL    equ    4

; 描述符
; usage: Descriptor Base, Limit, Attr
;        Base:  dd
;        Limit: dd (low 20 bits available)
;        Attr:  dw (lower 4 bits of higher byte are always 0)
%macro Descriptor 3	                          ; 段基址， 段界限， 段属性
    dw    %2 & 0xFFFF                         ; 段界限1
    dw    %1 & 0xFFFF                         ; 段基址1
    db    (%1 >> 16) & 0xFF                   ; 段基址2
    dw    ((%2 >> 8) & 0xF00) | (%3 & 0xF0FF) ; 属性1 + 段界限2 + 属性2
    db    (%1 >> 24) & 0xFF                   ; 段基址3
%endmacro                                     ; 共 8 字节

; 门
; usage: Gate Selector, Offset, DCount, Attr
;        Selector:  dw
;        Offset:    dd
;        DCount:    db
;        Attr:      db
%macro Gate 4
    dw    (%2 & 0xFFFF)                      ; 偏移地址1
    dw    %1                                 ; 选择子
    dw    (%3 & 0x1F) | ((%4 << 8) & 0xFF00) ; 属性
    dw    ((%2 >> 16) & 0xFFFF)              ; 偏移地址2
%endmacro 

