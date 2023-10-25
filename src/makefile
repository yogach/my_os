
.PHONY : all clean rebuild

KERNEL_SRC := kmain.c \
              screen.c \
              kernel.c \
              utility.c \
              task.c    \
              interrupt.c \
              ihandler.c \
              list.c    \
              queue.c   \
              memory.c  \
              mutex.c   \
              keyboard.c \
              event.c  \
              sysinfo.c

APP_SRC := screen.c   \
            utility.c  \
            app.c      \
            list.c    \
            queue.c   \
            memory.c  \
            syscall.c \
            demo1.c   \
            demo2.c   \
            shell.c
              

KERNEL_ADDR := B000
APP_ADDR     := F000
IMG := D.T.OS
#挂载目录
IMG_PATH := /mnt/hgfs

DIR_DEPS := deps
DIR_EXES := exes
DIR_OBJS := objs

DIRS := $(DIR_DEPS) $(DIR_EXES) $(DIR_OBJS)

KENTRY_SRC := kentry.asm
AENTRY_SRC     := aentry.asm
BLFUNC_SRC := blfunc.asm
LOADER_SRC := loader.asm
COMMON_SRC := common.asm
BOOT_SRC := boot.asm



BOOT_OUT := boot
LOADER_OUT := loader
KERNEL_OUT := kernel
APP_OUT     := app
KENTRY_OUT := $(DIR_OBJS)/kentry.o
AENTRY_OUT  := $(DIR_OBJS)/aentry.o

#定义编译Kernel的中间文件 放入到DIR_EXES内
KERNEL_EXE := kernel.out
KERNEL_EXE := $(addprefix $(DIR_EXES)/, $(KERNEL_EXE))
#将KERNEL_SRC指定的文件.c后缀替换成.o
KERNEL_OBJS := $(KERNEL_SRC:.c=.o)
#KERNEL_OBJS 上增加一个文件夹前缀
KERNEL_OBJS := $(addprefix $(DIR_OBJS)/, $(KERNEL_OBJS))
#.c后缀替换成.dep
KERNEL_DEPS := $(KERNEL_SRC:.c=.dep)
#DEPS上增加一个文件夹前缀
KERNEL_DEPS := $(addprefix $(DIR_DEPS)/,$(KERNEL_DEPS))

#定义编译App的中间文件 放入到DIR_EXES内
APP_EXE := app.out
APP_EXE := $(addprefix $(DIR_EXES)/, $(APP_EXE))
#将KERNEL_SRC指定的文件.c后缀替换成.o
APP_OBJS := $(APP_SRC:.c=.o)
#KERNEL_OBJS 上增加一个文件夹前缀
APP_OBJS := $(addprefix $(DIR_OBJS)/, $(APP_OBJS))
#.c后缀替换成.dep
APP_DEPS := $(APP_SRC:.c=.dep)
#DEPS上增加一个文件夹前缀
APP_DEPS := $(addprefix $(DIR_DEPS)/,$(APP_DEPS))


all : $(DIRS)  $(IMG) $(BOOT_OUT) $(LOADER_OUT) $(KERNEL_OUT) $(APP_OUT)
	@echo "Build Success ==> D.T.OS!"

#指定参数all和不指定参数时 需要包含$(DEPS)
ifeq ("$(MAKECMDGOALS)", "all")
-include $(KERNEL_DEPS) 
-include $(APP_DEPS)
endif

ifeq ("$(MAKECMDGOALS)", "")
-include $(KERNEL_DEPS) 
-include $(APP_DEPS)
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

#使用elf2obj 提取文件内的代码段和数据段 -c指定的是$@在内存中的加载地址
$(KERNEL_OUT) : $(KERNEL_EXE)
	./elf2kobj -c$(KERNEL_ADDR) $< $@
	sudo mount -o loop $(IMG) $(IMG_PATH)
	sudo cp $@ $(IMG_PATH)/$@
	sudo umount $(IMG_PATH)

$(KERNEL_EXE) : $(KENTRY_OUT) $(KERNEL_OBJS) 
	ld -s $^ -o $@


$(AENTRY_OUT) : $(AENTRY_SRC) $(COMMON_SRC)
	nasm -f elf $< -o $@

#
$(APP_OUT) : $(APP_EXE)
	./elf2kobj -c$(APP_ADDR) $< $@
	sudo mount -o loop $(IMG) $(IMG_PATH)
	sudo cp $@ $(IMG_PATH)/$@
	sudo umount $(IMG_PATH)

$(APP_EXE) : $(AENTRY_OUT) $(APP_OBJS) 
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
	rm -fr $(IMG) $(BOOT_OUT) $(LOADER_OUT) $(KERNEL_OUT) $(APP_OUT) $(DIRS)
	
rebuild :
	@$(MAKE) clean
	@$(MAKE) all
