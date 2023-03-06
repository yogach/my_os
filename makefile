
.PHONY : all clean rebuild

KERNEL_ADDR := B000
IMG := D.T.OS
IMG_PATH := /mnt/hgfs

DIR_DEPS := deps
DIR_EXES := exes
DIR_OBJS := objs

DIRS := $(DIR_DEPS) $(DIR_EXES) $(DIR_OBJS)

KENTRY_SRC := kentry.asm
BLFUNC_SRC := blfunc.asm
LOADER_SRC := loader.asm
COMMON_SRC := common.asm
BOOT_SRC := boot.asm

KERNEL_SRC := kmain.c

BOOT_OUT := boot
LOADER_OUT := loader
KERNEL_OUT := kernel
KENTRY_OUT := $(DIR_OBJS)/kentry.o

#定义最终生成的目标名 并加上文件夹名
EXE := kernel.out
EXE := $(addprefix $(DIR_EXES)/, $(EXE))

#得到当前目录下的所有.c后缀文件
SRCS := $(wildcard *.c)

#将.c后缀替换成.o
OBJS := $(SRCS:.c=.o)

#OBJS上增加一个文件夹前缀
OBJS := $(addprefix $(DIR_OBJS)/, $(OBJS))

#.c后缀替换成.dep
DEPS := $(SRCS:.c=.dep)

#DEPS上增加一个文件夹前缀
DEPS := $(addprefix $(DIR_DEPS)/,$(DEPS))


all : $(DIRS)  $(IMG) $(BOOT_OUT) $(LOADER_OUT) $(KERNEL_OUT)
	@echo "Build Success ==> D.T.OS!"

#指定参数all和不指定参数时 需要包含$(DEPS)
ifeq ("$(MAKECMDGOALS)", "all")
-include $(DEPS)
endif

ifeq ("$(MAKECMDGOALS)", "")
-include $(DEPS)
endif

$(IMG) :
	bximage $@ -q -fd -size=1.44
	
$(BOOT_OUT) : $(BOOT_SRC) $(BLFUNC_SRC)
	nasm $< -o $@
	dd if=$@ of=$(IMG) bs=512 count=1 conv=notrunc
	
$(LOADER_OUT) : $(LOADER_SRC) $(COMMON_SRC) $(BLFUNC_SRC)
	nasm $< -o $@
	sudo mount -o loop $(IMG) $(IMG_PATH)
	sudo cp $@ $(IMG_PATH)/$@
	sudo umount $(IMG_PATH)

$(KENTRY_OUT) : $(KENTRY_SRC) $(COMMON_SRC)
	nasm -f elf $< -o $@

$(KERNEL_OUT) : $(EXE)
	./elf2kobj -c$(KERNEL_ADDR) $< $@
	sudo mount -o loop $(IMG) $(IMG_PATH)
	sudo cp $@ $(IMG_PATH)/$@
	sudo umount $(IMG_PATH)

$(EXE) : $(KENTRY_OUT) $(OBJS) 
	ld -s $^ -o $@

$(DIR_OBJS)/%.o : %.c
	gcc -fno-builtin -fno-stack-protector -o $@ -c $(filter %.c, $^)

$(DIRS) :
	mkdir $@

#gcc -MM 生成依赖 使用sed加上objs/前缀 最后输出到target中
#$(DIR_DEPS) 也作为依赖生成 在include找不到$(DIR_DEPS)时 自动创建
#使用ifeq判断当前目录下是否存在$(DIR_DEPS)文件夹 
#可以避免因为生成.dep文件时,重复修改$(DIR_DEPS)的时间，导致依赖重复创建的问题
#增加deps文件对%.c文件的依赖
#比如生成的deps的内容是这样objs/const.o deps/const.dep : const.c 
#当const.c被更新时 deps/const.dep也会被更新
ifeq ("$(wildcard $(DIR_DEPS))", "")
$(DIR_DEPS)/%.dep : $(DIR_DEPS) %.c
else
$(DIR_DEPS)/%.dep : %.c
endif
	@echo "Creating $@ ..."
	@set -e; \
	gcc -MM -E $(filter %.c, $^) | sed 's,\(.*\)\.o[ :]*,objs/\1.o $@ : ,g' > $@
	
clean :
	rm -fr $(IMG) $(BOOT_OUT) $(LOADER_OUT) $(KERNEL_OUT) $(DIRS)
	
rebuild :
	@$(MAKE) clean
	@$(MAKE) all
