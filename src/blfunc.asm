jmp short _start  ;头信息起始地址之前还有2字节
nop

; fat12文件系统开头的数据分布	
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

const:
	RootEntryOffset  equ 19
	RootEntryLength  equ 14
	SPInitValue      equ BaseOfStack - EntryItemLength   ;栈起始地址
	EntryItem        equ SPInitValue 
	EntryItemLength  equ 32 ;目标文件根目录信息长度
	FatEntryOffset   equ 1  ;fat表逻辑扇区号
	FatEntryLength   equ 9
	
_start:
    jmp BLMain

; LoadTarget的执行位置是在16位模式下 所以一个指针占的大小是2字节	
; unshort LoadTarget( char*   Target,      notice ==> sizeof(char*) == 2
;                     unshort TarLen,
;                     unshort BaseOfTarget,
;                     unshort BOT_Div_0x10,
;                     char*   Buffer );
; return:
;     dx --> (dx != 0) ? success : failure    
LoadTarget:
        mov bp, sp
 
        ;将根目录区拷贝到内存
	mov ax, RootEntryOffset
	mov cx, RootEntryLength
	mov bx, [bp + 10] ; mov bx, Buffer
	
	call ReadSector 
	
	;查找目标文件在目录中是否存在
	mov si, [bp + 2] ;mov si, Target
	mov cx, [bp + 4] ;mov cx, TarLen
	mov dx, 0
	
	call FindEntry 
	
	;如果找不到则跳转到失败
	cmp dx, 0
	jz finish
	
	;将目标文件根目录信息拷贝到指定区域
        mov si, bx
	mov di, EntryItem
	mov cx, EntryItemLength
	
	call MemCpy
	
	;计算fat表的长度 然后拷贝到BaseOfTarget的前面
	mov bp, sp
	mov ax, FatEntryLength
	mov cx, [BPB_BytsPerSec]
	mul cx 
	mov bx, [bp + 6] ;mov bx, BaseOfTarget
	sub bx, ax
	
	mov ax, FatEntryOffset
	mov cx, FatEntryLength
	
	call ReadSector
	
	mov dx, [EntryItem + 0x1A]
	mov es, [bp + 8] ;mov si, BaseOfTarget / 0x10 ;重新设置es si 定位到要拷贝的位置
	                 ;mov es, si 
	xor si, si  ;异或操作 相当于清0 那为什么 mov si, 0不行？

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
	jnb finish 
	add si, 512   ;指向下一个扇区
	
	;因为si的最大值为0xffff 拷贝的最大大小为64K 以下的代码就为了解除这个限制
	cmp si, 0      ;将si与0做比较 确定si是否已经溢出 si的最大值为0xffff
	jnz continue

	;如果溢出 则es的递增
	mov si, es
	add si, 0x1000 ;将es值加 0x1000 实际上指向地址增加了 0x10000
	mov es, si
	mov si, 0

continue:	
	jmp loading
	
finish:
    ret	
	
; cx --> index FatVec下标
; bx -->fat table addrss
;
; return:
;      dx --> fat[index]
FatVec:    
    ;Fat表起始字节 = FatVec下标 / 2 * 3
    push cx
    
    mov ax, cx
    shr ax, 1   ; ax 右移1位 等同于除2	

	mov cx, 3
	mul cx
	mov cx, ax ;Fat表起始字节 保存在cx中
	
	pop ax
	
	;判断余数 最低位为1 代表时奇数
	and ax, 1
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
	
; ax    --> logic sector number
; cx    --> number of sector
; es:bx --> target address	
ReadSector:
       
	mov ah, 0x00
	mov dl, [BS_DrvNum]
	int 0x13
	
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
