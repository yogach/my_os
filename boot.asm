org 0x7c00 ;起始地址

jmp short start  ;头信息起始地址之前还有2字节
nop

define:
    BaseOfStack equ 0x7c00 
	BaseOfLoader equ 0x9000
	RootEntryOffset  equ 19
	RootEntryLength  equ 14
	EntryItemLength  equ 32 ;目标文件根目录信息长度
	FatEntryOffset   equ 1  ;fat表逻辑扇区号
	FatEntryLength   equ 9
	
	
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
    
	;将根目录区拷贝到内存
	mov ax, RootEntryOffset
	mov cx, RootEntryLength
	mov bx, Buf
	
	call ReadSector 
	
	;查找目标文件在目录中是否存在
	mov si, MsgStr2
	mov cx, MsgLen2
	mov dx, 0
	
	call FindEntry 
	
	;如果找不到则跳转到失败
	cmp dx, 0
	jz output
	
	;将目标文件根目录信息拷贝到指定区域
    mov si, bx
	mov di, EntryItem
	mov cx, EntryItemLength
	
	call MemCpy
	
	;计算fat表的长度 然后拷贝到BaseOfLoader的前面
	mov ax, FatEntryLength
	mov cx, [BPB_BytsPerSec]
	mul cx 
	mov bx, BaseOfLoader
	sub bx, ax
	
	mov ax, FatEntryOffset
	mov cx, FatEntryLength
	
	call ReadSector
	
	mov dx, [EntryItem + 0x1A]
	mov si, BaseOfLoader
	
loading:
	mov ax, dx  ;dx保存的是目标文件数据的从0开始的偏移簇号
	add ax, 31  ;加31 得到数据实际的开始簇号 
	mov cx, 1
	push dx
	push bx
	mov bx, si  ;si指向目标位置
	call ReadSector
	pop bx
	pop cx       ;cx为偏移簇号
	call FatVec  ;读取fat 得到下一个簇号
	cmp dx, 0xff7    ;如果大于这个值 则代表文件内容已经读取完毕
	jnb BaseOfLoader 
	add si, 512   ;指向下一个扇区
	jmp loading
	
output:
    ;mov bp, BaseOfLoader
	;mov cx, [EntryItem + 0x1C]
	mov bp, MsgStr
	mov cx, MsgLen
	call print
	
last:
    hlt     ;休眠一段时间
    jmp last

; cx --> index FatVec下标
; bx -->fat table addrss
;
; return:
;      dx --> fat[index]
FatVec:    
    ;Fat表起始字节 = FatVec下标 / 2 * 3
	mov ax, cx
	mov cl, 2
	div cl  

	push ax ;保存除法结果
	
	mov ah, 0
	mov cx, 3
	mul cx
	mov cx, ax ;Fat表起始字节 保存在cx中
	
	pop ax
	
	;判断余数
	cmp ah, 0
	jz even
	jmp odd
	
even: ; FatVec[j] = ( (Fat[i+1] & 0x0F) << 8 ) | Fat[i];
	mov dx, cx
	add dx, 1
	add dx, bx  ;dx指向对应的fat表项地址 
	mov bp, dx  
	mov dl, byte [bp] ;得到对应数据
	and dl, 0x0F
	shl dx, 8
	add cx, bx
	mov bp, cx
	or dl, byte [bp]
	jmp return 
	
odd: ; FatVec[j+1] = (Fat[i+2] << 4) | ( (Fat[i+1] >> 4) & 0x0F );
	mov dx, cx
	add dx, 2
	add dx, bx
	mov bp, dx
	mov dl, byte [bp]
	mov dh, 0
	shl dx, 4 ;左移4位
	add cx, 1
	add cx, bx
	mov bp, cx
	mov cl, byte [bp]
	shr cl, 4 ;右移4位
	and cl, 0x0F
	mov ch, 0
	or dx, cx 
	

return:
	ret
	
	
; ds:si --> source
; es:di --> destination
; cx    --> length
MemCpy:
	
	cmp si, di ;对比源地址和目标地址前后顺序
	
	ja btoe
	
	;进入此处代表需要从后往前拷贝
	add si, cx
	add di, cx
	dec si
	dec di
	
	jmp etob
	
btoe:
	cmp cx, 0
	jz done
	mov al, [si]
	mov byte [di], al
	inc si
	inc di
	dec cx 
	jmp btoe

etob:
	cmp cx, 0
	jz done
	mov al, [si]
	mov byte [di], al
	dec si
	dec di 
	dec cx
	jmp etob

done:
	ret

; es:bx --> root entry offset address
; ds:si --> target string
; cx    --> target length
;
; return:
;     (dx != 0) ? exist : noexist
;        exist --> bx is the target entry
FindEntry:
	push cx
	
	mov dx, [BPB_RootEntCnt] ;dx保存的是分区内最多保存多少文件数
	mov bp, sp
	
find:
	cmp dx, 0
	jz noexist
	mov di, bx
	mov cx, [bp] ;从栈上得到比较长度
        push si
	call MemCmp
        pop si
	cmp cx, 0
	jz exist
	add bx, 32 ;bx指向下一个文件的根目录文件信息
	dec dx
	jmp find
	
exist:
noexist:
	pop cx
	
	ret

; ds:si --> source
; es:di --> destination
; cx    --> length
;
; return :
;	     (cx == 0) ? equal : noequal
MemCmp:

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
	
	ret 
;es:bp --> string address
;cx    --> string length
print:
    mov dx, 0
    mov ax, 0x1301
    mov bx, 0x0007
    int 0x10    ;调用bios中断打印字符串
    ret

;no parameter
ResetFloppy:
	
	mov ah, 0x00
	mov dl, [BS_DrvNum]
	int 0x13

	ret
	
; ax    --> logic sector number
; cx    --> number of sector
; es:bx --> target address	
ReadSector:
	
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
	
	ret
	


MsgStr db "No LOADER ..."
MsgLen equ ($-MsgStr) ;$代表当前行地址 结果是得到MsgStr的长度
MsgStr2 db "LOADER     "  
MsgLen2 equ ($-MsgStr2)
EntryItem times EntryItemLength db 0x00 ;定义一块区域
Buf:
    times 510-($-$$) db 0x00 ;$为当前行地址 $$为代码起始行地址 作用是填充数据到512字节
    db 0x55, 0xaa ;结尾填上主引导程序标识