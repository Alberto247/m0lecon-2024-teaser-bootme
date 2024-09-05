#!/bin/sh
qemu-system-x86_64 -drive format=raw,file=bootloader.bin -drive format=raw,file=flash.img -nographic