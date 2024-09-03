#!/bin/bash

mkfs.ext2 -L "FLASH001" flash.img

sudo mount -t ext2 -o loop,rw flash.img /mnt/dfs

sudo cp -r fs/* /mnt/dfs/

sudo umount /mnt/dfs