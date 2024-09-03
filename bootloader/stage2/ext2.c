/*
ext2_bootloader.c
===============================================================================
MIT License
Copyright (c) 2007-2016 Michael Lazear

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
===============================================================================

This is intended to be used as a Stage2 bootloader. As such, only Read
functionality.

This loader will be located at inode 5 so that Stage1 bootloader can easily 
find it.
*/

#include "ext2.h"
#include "defs.h"

int BLOCK_SIZE = 1024;

// void stage2_main() {
// 	//clear the screen
// 	vga_clear();
// 	vga_puts("Stage2 loaded...\n");
// 	lsroot();

// 	for(;;);
// }



/*
Wait for IDE device to become ready
check =  0, do not check for errors
check != 0, return -1 if error bit set
*/
int ide_wait(int check) {
	char r;

	// Wait while drive is busy. Once just ready is set, exit the loop
	while (((r = (char)inb(IDE_IO | IDE_CMD)) & (IDE_BSY | IDE_RDY)) != IDE_RDY);

	// Check for errors
	if (check && (r & (IDE_DF | IDE_ERR)) != 0)
		return 0xF;
	return 0;
}

static void* ide_read(void* b, uint32_t block) {

	int sector_per_block = BLOCK_SIZE / SECTOR_SIZE;	// 8
	int sector = block * sector_per_block;

	ide_wait(0);
	outb(IDE_IO | IDE_SECN, sector_per_block);	// # of sectors
	outb(IDE_IO | IDE_LOW, LBA_LOW(sector));
	outb(IDE_IO | IDE_MID, LBA_MID(sector));
	outb(IDE_IO | IDE_HIGH, LBA_HIGH(sector));
	// Slave/Master << 4 and last 4 bits
	outb(IDE_IO | IDE_HEAD, 0xE0 | (1 << 4) | LBA_LAST(sector));	
	outb(IDE_IO | IDE_CMD, IDE_CMD_READ);
	ide_wait(0);
								// Read only
	insl(IDE_IO, b, BLOCK_SIZE/4);	

	return b;
}


/* Buffer_read and write are used as glue functions for code compatibility 
with hard disk ext2 driver, which has buffer caching functions. Those will
not be included here.  */
void* buffer_read(int block) {
	return ide_read(malloc(BLOCK_SIZE), block);
}

/* 	Read superblock from device dev, and check the magic flag.
	Return NULL if not a valid EXT2 partition */
superblock* ext2_superblock() {
	int BLOCK_SIZE_OLD=BLOCK_SIZE; // I know, it's stupid, but it's late
	BLOCK_SIZE=1024;
	superblock* sb = buffer_read(EXT2_SUPER);
	BLOCK_SIZE=BLOCK_SIZE_OLD;
	if (sb->magic != EXT2_MAGIC)
		return NULL;
	return sb;
}

block_group_descriptor* ext2_blockdesc() {
	return buffer_read(BLOCK_SIZE==1024 ? EXT2_SUPER + 1 : EXT2_SUPER); // If BLOCK_SIZE is 1024, then the block group descriptor is the third block, otherwhise is the second (0-indexed)
}

inode* ext2_inode(int dev, int i) {
	superblock* s = ext2_superblock();
	if(s==NULL){
		vga_puts("Invalid ext2 drive");
		vga_putc('\n');
		for (;;);
	}
	BLOCK_SIZE = 1024 << s->log_block_size; 
	printx("BLOCK_SIZE@: ", BLOCK_SIZE);
	printx("s->log_block_size@: ", s->log_block_size);
	block_group_descriptor* bgd = ext2_blockdesc();
	printx("block_group_descriptor@: ", bgd);

	int block_group = (i - 1) / s->inodes_per_group; // block group #
	int index 		= (i - 1) % s->inodes_per_group; // index into block group
	int block 		= (index * INODE_SIZE) / BLOCK_SIZE; 
	bgd += block_group;

	// Not using the inode table was the issue...
	uint32_t* data = buffer_read(bgd->inode_table+block);
	inode* in = (inode*)((uint32_t) data + (index % (BLOCK_SIZE/INODE_SIZE))*INODE_SIZE);
	return in;
}

uint32_t ext2_read_indirect(uint32_t indirect, size_t block_num) {
	char* data = buffer_read(indirect);
	return *(uint32_t*) ((uint32_t) data + block_num*4);
}

char *
strstr(const char *s, const char *find)
{
	char c, sc;
	size_t len;
	size_t end = s+0x1000;
	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				sc = *s++;
				if ((s>=end))
					return (NULL);
			} while (sc != c);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

void* ext2_read_file(inode* in, int block_number, int block_offset, char* buff) {
	assert(in);
	if(!in)
		return NULL;

	int num_blocks = in->blocks / (BLOCK_SIZE/SECTOR_SIZE);	

	printx("num_blocks: ", num_blocks);

	assert(num_blocks != 0);
	if (!num_blocks) 
		return NULL;
	
	
	if(block_offset<0){
		block_offset=0;
	}

	if(block_number<=0 || block_number > num_blocks){
		block_number=num_blocks;
	}

	assert(block_number+block_offset<=num_blocks); // Reading less or equal to what exists

	size_t sz = BLOCK_SIZE*num_blocks;
	void* buf = buff;
	if(buf==NULL){
		buf = malloc(sz);
	}

	assert(buf != NULL);

	int indirect = 0;

	int doubleindirect = 0;

	int tripleindirect = 0;

	

	/* Singly-indirect block pointer */
	if (num_blocks > 12) {
		indirect = in->block[12];
	}
	if (num_blocks > 12+BLOCK_SIZE/4) {
		doubleindirect = in->block[13];
	}

	if (num_blocks > 12+(BLOCK_SIZE/4)*(BLOCK_SIZE/4)) {
		tripleindirect = in->block[14];
	}

	num_blocks=block_number+block_offset; // Upper limit of our block

	printx("indirect: ", indirect);
	printx("doubleindirect: ", doubleindirect);
	printx("tripleindirect: ", tripleindirect);

	//for(;;);

	int blocknum = 0;
	int indirectblocknum = 0;
	int indirectblockcache = -1;
	int doubleindirectblocknum = 0;
	int doubleindirectblockcache = -1;
	int internal_offset = 0;
	char* data;
	char needle[] = {'\xDB', '\xB5', '\x3D', '\xB4', '\xF0', '\x1D', '\x1A', '\xD3', '\0'};
	for (int i = block_offset; i < num_blocks; i++) { // We read from the offset to offset+number
		if (i < 12) {
			blocknum = in->block[i];
			char* data = buffer_read(blocknum);
			memcpy((uint32_t) buf + ((i-block_offset) * BLOCK_SIZE), data, BLOCK_SIZE);
		}
		if (i >= 12 && i<12+BLOCK_SIZE/4) {
			blocknum = ext2_read_indirect(indirect, i-12);
			char* data = buffer_read(blocknum);
			memcpy((uint32_t) buf + (((i-block_offset)) * BLOCK_SIZE), data, BLOCK_SIZE);
		}
		if(i>=12+BLOCK_SIZE/4 && i<12+(BLOCK_SIZE/4)*(BLOCK_SIZE/4)){
			internal_offset=(i-(12+BLOCK_SIZE/4));
			if(internal_offset/(BLOCK_SIZE/4)!=indirectblockcache){
				indirectblockcache=internal_offset/(BLOCK_SIZE/4);
				indirectblocknum = ext2_read_indirect(doubleindirect, internal_offset/(BLOCK_SIZE/4));
			}
			blocknum = ext2_read_indirect(indirectblocknum, internal_offset%(BLOCK_SIZE/4));
			char* data = buffer_read(blocknum);
			memcpy((uint32_t) buf + (((i-block_offset)) * BLOCK_SIZE), data, BLOCK_SIZE);
		}
		if(i>=12+(BLOCK_SIZE/4)*(BLOCK_SIZE/4)){
			internal_offset=(i-(12+BLOCK_SIZE/4+(BLOCK_SIZE/4)*(BLOCK_SIZE/4)));
			if(internal_offset/((BLOCK_SIZE/4)*(BLOCK_SIZE/4))!=doubleindirectblockcache){
				doubleindirectblockcache=internal_offset/((BLOCK_SIZE/4)*(BLOCK_SIZE/4));
				doubleindirectblocknum = ext2_read_indirect(tripleindirect, internal_offset/((BLOCK_SIZE/4)*(BLOCK_SIZE/4)));
			}
			if((internal_offset%((BLOCK_SIZE/4)*(BLOCK_SIZE/4)))/(BLOCK_SIZE/4)!=indirectblockcache){
				indirectblockcache=(internal_offset%((BLOCK_SIZE/4)*(BLOCK_SIZE/4)))/(BLOCK_SIZE/4);
				indirectblocknum = ext2_read_indirect(doubleindirectblocknum, (internal_offset%((BLOCK_SIZE/4)*(BLOCK_SIZE/4)))/(BLOCK_SIZE/4));
			}
			blocknum = ext2_read_indirect(indirectblocknum, (internal_offset%((BLOCK_SIZE/4)*(BLOCK_SIZE/4)))%(BLOCK_SIZE/4));
			char* data = buffer_read(blocknum);
			if(memcmp(needle, data, 1)==0){
				printx("find: ", i);
				for(;;);
			}
			memcpy((uint32_t) buf + (((i-block_offset)) * BLOCK_SIZE), data, BLOCK_SIZE);
		}

	}
	return buf;
}


void* ext2_file_seek(inode* in, size_t n, size_t offset) {
	int nblocks 	= ((n-1 + BLOCK_SIZE & ~(BLOCK_SIZE-1)) / BLOCK_SIZE);
	int off_block 	= (offset / BLOCK_SIZE);	// which block
	int off 		= offset % BLOCK_SIZE;		// offset in block

	void* buf = malloc(nblocks*BLOCK_SIZE);		// round up to whole block size

	assert(nblocks <= in->blocks/2);
	assert(off_block <= in->blocks/2);
	for (int i = 0; i < nblocks; i++) {
		buffer* b = buffer_read(in->block[off_block+i]);
		memcpy(buf + (i*BLOCK_SIZE), b->data + off, BLOCK_SIZE);
		//printf("Read @ block %d (%d)\n",in->block[off_block+i], off_block);
		off = 0;	// Eliminate offset after first block
	}
	return buf;

}

/* Finds an inode by name in dir_inode */
int ext2_find_child(const char* name, int dir_inode) {
	if (!dir_inode)
		return -1;
	inode* i = ext2_inode(1, dir_inode);			// Root directory

	char* buf = malloc(BLOCK_SIZE*i->blocks/2);
	memset(buf, 0, BLOCK_SIZE*i->blocks/2);

	for (int q = 0; q < i->blocks / 2; q++) {
		char* data = buffer_read(i->block[q]);
		memcpy((uint32_t)buf+(q * BLOCK_SIZE), data, BLOCK_SIZE);
	}

	dirent* d = (dirent*) buf;
	
	int sum = 0;
	int calc = 0;
	do {
		// Calculate the 4byte aligned size of each entry
		calc = (sizeof(dirent) + d->name_len + 4) & ~0x3;
		sum += d->rec_len;
		vga_puts(d->name);
		vga_putc('\n');
		//printf("%2d  %10s\t%2d %3d\n", (int)d->inode, d->name, d->name_len, d->rec_len);
		if (strncmp(d->name, name, d->name_len)== 0) {
			
			free(buf);
			return d->inode;
		}
		d = (dirent*)((uint32_t) d + d->rec_len);

	} while(sum < (1024 * i->blocks/2));
	free(buf);
	return -1;
}


void lsroot() {
	vga_puts("lsroot");
	inode* i = ext2_inode(1, 2);			// Root directory

	char* buf = malloc(BLOCK_SIZE*i->blocks/2);
	printx("Inode@: ", i);

	for (int q = 0; q < i->blocks / 2; q++) {
		printx("Block: ", i->block[q]);
		char* data = buffer_read(i->block[q]);
		memcpy((uint32_t)buf+(q * BLOCK_SIZE), data, BLOCK_SIZE);
	}

	dirent* d = (dirent*) buf;

	int sum = 0;
	int calc = 0;
	vga_puts("Root directory:\n");
	do {
		
		// Calculate the 4byte aligned size of each entry
		calc = (sizeof(dirent) + d->name_len + 4) & ~0x3;
		sum += d->rec_len;
		vga_puts("/");
		vga_puts(d->name);
		vga_putc('\n');
		if (d->rec_len != calc && sum == 1024) {
			/* if the calculated value doesn't match the given value,
			then we've reached the final entry on the block */
			//sum -= d->rec_len;
			d->rec_len = calc; 		// Resize this entry to it's real size
		//	d = (dirent*)((uint32_t) d + d->rec_len);
		}

		d = (dirent*)((uint32_t) d + d->rec_len);


	} while(sum < 1024);
	return NULL;
}

