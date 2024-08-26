all: bootloader.s bootloader.c display.h linker
	nasm -f elf32 bootloader.s -o bootloader.o
	gcc -ffreestanding -m16 -fno-PIE -c bootloader.c -o bootloader2.o
	ld -melf_i386 --build-id=none -T linker bootloader2.o bootloader.o -o bootloader.elf
	objcopy -O binary bootloader.elf bootloader.bin
	truncate -s 1K bootloader.bin