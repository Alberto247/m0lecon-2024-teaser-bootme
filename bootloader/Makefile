COBJS	= 	stage2/main.o \
			stage2/ext2.o \
			stage2/lib.o \
			stage2/vga.o

CC = gcc
LD = ld
AS = nasm
CCFLAGS	= -w -fno-pic -fno-builtin -nostdlib -ffreestanding -std=gnu99 -m32 -c 
EXT2UTIL= ../ext2util/ext2util


%.o : %.c
	$(CC) $(CCFLAGS) $< -o $@


stage2: $(COBJS)


all: stage2
	$(AS) -f bin stage1/mbr.s -o stage1.bin
	$(LD) -melf_i386 -N -e stage2_main -Ttext 0x10000 -o stage2.bin $(COBJS) --oformat binary
	truncate -s 20K stage2.bin
	cat stage1.bin stage2.bin > bootloader.bin

clean:
	rm stage2/*.o
	rm stage2.*
	rm stage1.*