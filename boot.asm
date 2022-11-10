org 0x7c00 ;起始地址

jmp short start  ;头信息起始地址之前还有2字节
nop

define:
    BaseOfStack equ 0x7c00 
	RootEntryOffset  equ 19
	RootEntryLength  equ 14
	
header:
    BS_OEMName     db "D.T.Soft"
    BPB_BytsPerSec dw 512
    BPB_SecPerClus db 1
    BPB_RsvdSecCnt dw 1
    BPB_NumFATs    db 2
    BPB_RootEntCnt dw 224
    BPB_TotSec16   dw 2880
    BPB_Media      db 0xF0
    BPB_FATSz16    dw 9
    BPB_SecPerTrk  dw 18
    BPB_NumHeads   dw 2
    BPB_HiddSec    dd 0
    BPB_TotSec32   dd 0
    BS_DrvNum      db 0
    BS_Reserved1   db 0
    BS_BootSig     db 0x29
    BS_VolID       dd 0
    BS_VolLab      db "D.T.OS-0.01"
    BS_FileSysType db "FAT12   "	

start:
    ;初始化各个段寄存器  
    mov ax, cs  ;cs为代码段起始 段地址
    mov ss, ax
	mov ds, ax
	mov es, ax
	mov sp, BaseOfStack ;设置栈地址
    
	mov ax, RootEntryOffset
	mov cx, RootEntryLength
	mov bx, Buf
	
	call ReadSector ;将根目录区拷贝到内存
	
	mov si, MsgStr2
	mov cx, MsgLen2
	mov dx, 0
	
	call FindEntry
	
	cmp dx, 0
	jz notfind
	jmp last
	
notfind:
    mov bp, MsgStr
	mov cx, MsgLen
	call print
	
last:
    hlt     ;休眠一段时间
    jmp last

; es:bx --> root entry offset address
; ds:si --> target string
; cx    --> target length
;
; return:
;     (dx != 0) ? exist : noexist
;        exist --> bx is the target entry
FindEntry:
	push di
	push bp
	push cx
	
	mov dx, [BPB_RootEntCnt] ;dx保存的是分区内最多保存多少文件数
	mov bp, sp
	
find:
	cmp dx, 0
	jz noexist
	mov di, bx
	mov cx, [bp] ;从栈上得到比较长度
	call MemCmp
	cmp cx, 0
	jz exist
	add bx, 32 ;bx指向下一个文件的根目录文件信息
	dec dx
	jmp find
	
exist:
noexist:
	pop cx
	pop bp
	pop di
	
	ret

; ds:si --> source
; es:di --> destination
; cx    --> length
;
; return :
;	     (cx == 0) ? equal : noequal
MemCmp:
	push si
	push di
	push ax

compare:
	cmp cx, 0 
	jz equal 
	mov al, [si]
	cmp al, byte [di] ;按字节进行比较
	jz goon
	jmp noequal
goon:
	inc si ;进入这里代表此次对比成功
	inc di 
	dec cx
	jmp compare

equal:
noequal:
	pop ax
	pop di
	pop si
	
	ret 
;es:bp --> string address
;cx    --> string length
print:
    mov ax, 0x1301
    mov bx, 0x0007
    int 0x10    ;调用bios中断打印字符串
    ret

;no parameter
ResetFloppy:
    push ax
	push dx
	
	mov ah, 0x00
	mov dl, [BS_DrvNum]
	int 0x13
	
	pop dx
	pop ax
	
	ret
	
; ax    --> logic sector number
; cx    --> number of sector
; es:bx --> target address	
ReadSector:
	push bx
	push cx
	push dx
	push ax
	
	call ResetFloppy
	
	push bx
	push cx
	
	mov bl, [BPB_SecPerTrk]
	div bl    ;执行除法 被除数在ax里
	mov cl, ah ; 除法余数位于ah cl-起始扇区号
	add cl, 1
	mov ch, al ; 除法商位于al ch-柱面号
	shr ch, 1
	mov dh, al ;dh-磁头号
	and dh, 1
	mov dl, [BS_DrvNum] ;dl-驱动器号
	
	pop ax ; 实际上取得传入的cx的值 作为读取的扇区长度
	pop bx
	
	mov ah, 0x02

read:
	int 0x13
	jc read ; 当运算产生进位标志时，即CF=1时，跳转到目标程序处。
	
	pop ax
	pop dx
	pop cx
	pop bx
	
	ret
	


MsgStr db "No LOADER ..."
MsgLen equ ($-MsgStr) ;$代表当前行地址 结果是得到MsgStr的长度
MsgStr2 db "LOADER     "  ;无法定义成Target 很奇怪
MsgLen2 equ ($-MsgStr2)
Buf:
    times 510-($-$$) db 0x00 ;$为当前行地址 $$为代码起始行地址 作用是填充数据到512字节
    db 0x55, 0xaa ;结尾填上主引导程序标识