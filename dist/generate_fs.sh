#!/bin/bash

rm flash.img

touch flash.img

chown -R alber:alber flash.img

truncate -s 75M flash.img

mkfs.ext2 -b 4096 -L "FLASH001" flash.img

sudo mount -t ext2 -o loop,rw flash.img /mnt/dfs

sudo cp -r fs/* /mnt/dfs/

sudo umount /mnt/dfs