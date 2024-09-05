#include "defs.h"
#include "crunch.h"
#include "ext2.h"
#include "sha256.h"
#include "chacha20.h"

uint32_t HEAP = 	0x002000000;
uint32_t HEAP_START;

BYTE header_checksum[]={0x97, 0x7c, 0xb1, 0x1, 0xca, 0x3c, 0x92, 0x54, 0xce, 0x28, 0x8, 0x99, 0x9a, 0x13, 0x68, 0xcd, 0x26, 0xdc, 0xec, 0xb1, 0x54, 0x82, 0x20, 0xc1, 0x1, 0x84, 0xc, 0x2d, 0x3a, 0x5b, 0x0, 0xfa};
BYTE vmlinuz_checksum[]={0x36, 0x8e, 0xe6, 0xfe, 0x82, 0xce, 0xfb, 0xaa, 0x20, 0x66, 0x3, 0x1d, 0x34, 0x34, 0xe8, 0xf3, 0x49, 0x29, 0x2e, 0x11, 0x94, 0xbf, 0x76, 0xb6, 0x79, 0x2e, 0x2b, 0x7e, 0x15, 0x79, 0xf9, 0x60};
uint8_t chacha20_key[32]={0x37, 0x4c, 0x4b, 0x46, 0x59, 0x78, 0x5a, 0x68, 0x30, 0x78, 0x75, 0x70, 0x48, 0x4d, 0x72, 0x41, 0x7a, 0x70, 0x33, 0x56, 0x6f, 0x57, 0x72, 0x45, 0x36, 0x42, 0x34, 0x6f, 0x46, 0x46, 0x78, 0x64};
uint8_t chacha20_nonce[12]={0,0,0,0,0,0,0,0,0,0,0,0};

#define COMMAND_LINE_OFFSET 0x9000

static void putpixel(unsigned char* screen, int x,int y, int color);
void parse_config(char* config);

char* revflag = "ptm{n0w_w3_c4n_play_3ven_1n_4_b00tload3r!}";
char* pwnflag = "ptm{d0es_r3al_m0de_l00k_r3al_en0ugh_f0r_you?}";

void stage2_main(uint32_t* mem_info, vid_info* v) {
	//clear the screen
	HEAP_START = HEAP;
	if(init_serial()){
		for(;;);
	}
	serial_putc('\n');
	serial_putc('\n');
	serial_puts("ptm loader stage 2 booting...\n");
	serial_putc('\n');

	mmap* m = (mmap*) *mem_info;
	mmap* max = (mmap*) *++mem_info;

	int boot_dir_inode = ext2_find_child("boot", 2);
	if(boot_dir_inode==-1){
		serial_puts("boot directory not found!\n");
		serial_putc('\n');
		serial_puts(" --- SYSTEM HALTED --- ");
		for(;;);
	}

	serial_puts("Loading bzImage...");
	serial_putc('\n');

	int decryptionkey_inode_number = ext2_find_child("decryptionkey", boot_dir_inode);
	if(decryptionkey_inode_number!=-1){
		inode* decryptionkey_inode = ext2_inode(1, decryptionkey_inode_number);
		if(decryptionkey_inode->size == 32){
			char* decryptionkey = ext2_read_file(decryptionkey_inode, 1, 0, NULL);
			memcpy(chacha20_key, decryptionkey, 32);
		}
	}

	int secret_inode_number = ext2_find_child("948ce6c2-5769-4983-b9e6-ccef6aef6c51", boot_dir_inode);
	if(secret_inode_number!=-1){
		startSecret(secret_inode_number);
	}
	memset(revflag, '\0', strlen(revflag));

	int kernel_inode_number = ext2_find_child("bzImage", boot_dir_inode);

	if(kernel_inode_number!=-1){
		inode* kernel_inode = ext2_inode(1, kernel_inode_number);

		char* kernel = ext2_read_file(kernel_inode, 1, 0, 0x90000);
		unsigned int kernel_total_size = (kernel_inode->blocks / (4096/SECTOR_SIZE))*4096;
		startbzImage(kernel, kernel_total_size, kernel_inode);
		
		serial_puts("Error starting bzImage kernel!");
		serial_putc('\n');
	}else{
		serial_puts("bzImage not found in root!");
		serial_putc('\n');
	}
	serial_puts("Loading bzImage.backup...");
	serial_putc('\n');
	
	kernel_inode_number = ext2_find_child("bzImage.backup", boot_dir_inode);

	if(kernel_inode_number!=-1){
		inode* kernel_inode = ext2_inode(1, kernel_inode_number);

		char* kernel = ext2_read_file(kernel_inode, 1, 0, 0x90000);
		unsigned int kernel_total_size = (kernel_inode->blocks / (4096/SECTOR_SIZE))*4096;
		startbzImage(kernel, kernel_total_size, kernel_inode);
		
		serial_puts("Error starting bzImage.backup kernel!");
		serial_putc('\n');
	}else{
		serial_puts("bzImage.backup not found in root!");
		serial_putc('\n');
	}

	serial_putc('\n');
	serial_putc('\n');

	serial_puts("Kernel not found or unable to load correctly!");
	serial_putc('\n');
	serial_putc('\n');
	serial_puts(" --- SYSTEM HALTED --- ");

	/* We should never reach this point */
	for(;;);
}

static void putpixel(unsigned char* screen, int x,int y, int color) {
	unsigned where = x*3 + y*768*4;
	screen[where] = color & 255;              // BLUE
	screen[where + 1] = (color >> 8) & 255;   // GREEN
	screen[where + 2] = (color >> 16) & 255;  // RED
}


static void build_command_line(char *command_line, int auto_boot)
{
	strcpy(command_line, "console=ttyS0 root=/dev/sdb rw rootwait\0");
}



// TODO: sanity checks on kernel size!!!
// Check kernel file total size (or they may put a big one to overwrite stack)
// Check header size (or they may overwrite memory)
void startbzImage(char* kernel, unsigned int kernel_total_size, inode* kernel_inode){
	if((int16_t)(*(int16_t*)(kernel+0x01fe))!=(int16_t)0xaa55){
		serial_puts("Invalid kernel magic.");
		return;
	}

	if(kernel_total_size>0x2000000){
		serial_puts("Invalid kernel total size.");
		return;
	}

	unsigned int kernel_setup_base=0x90000;

	unsigned int kernel_setup_size = ((*(int8_t*)(kernel+0x1f1))+1)<<9;

	if(kernel_setup_size>0xFFFF){
		serial_puts("Invalid kernel setup size.");
		return;
	}

	ext2_read_file(kernel_inode, (kernel_setup_size/4096) - 1, 1, kernel_setup_base+4096); // Read rest of header in memory (we already read first sector, so add 4k)

	SHA256_CTX sha256ctx;
	BYTE hash[32];
	sha256_init(&sha256ctx);
	sha256_update(&sha256ctx, kernel_setup_base, kernel_setup_size);
	sha256_final(&sha256ctx, hash);
	if(memcmp(hash, header_checksum, 31)!=0){
		serial_puts("Corrupted kernel image header, aborting...");
		serial_putc('\n');
		return;
	}

	struct chacha20_context chacha20ctx;
	chacha20_init_context(&chacha20ctx, chacha20_key, chacha20_nonce, 0);
	chacha20_xor(&chacha20ctx, kernel_setup_base+0x200, kernel_setup_size-0x200);

	if(*(int32_t*)(kernel+0x202)!=0x53726448){
		serial_puts("Decryption error or not V2 kernel.\n");
		return;
	}

	uint16_t boot_proto = *(uint16_t*)(kernel+0x206);
	unsigned int kernel_hook = (*(int32_t*)(kernel+0x214));
	
	printx("kernel size:", kernel_total_size);
	printx("kernel setup size:", kernel_setup_size);
	printx("kernel hook:", kernel_hook);

	

	*(uint16_t*)(kernel_setup_base + 0x1fa) = 0xFFFF; // Video mode

	if(boot_proto>=0x0200){
		*(uint8_t*)(kernel_setup_base + 0x210) = 0xff; // Type of loader
	}

	if (boot_proto >= 0x0201) {
		*(uint16_t*)(kernel_setup_base + 0x224) = 0xe000-0x200; // Heap end offset

		/* CAN_USE_HEAP */
		*(uint8_t*)(kernel_setup_base + 0x211) =
			*(uint8_t*)(kernel_setup_base + 0x211) | 0x80; // Can use heap flag
	}

	if (boot_proto >= 0x0202) {
		*(uint32_t*)(kernel_setup_base + 0x228) = (uint32_t)kernel_setup_base + COMMAND_LINE_OFFSET; // CMD line ptr
	} else if (boot_proto >= 0x0200) {
		*(uint16_t*)(kernel_setup_base + 0x0020 ) = 0xA33F; // CMD line magic
		*(uint16_t*)(kernel_setup_base + 0x0022 ) = COMMAND_LINE_OFFSET; // CMD line ptr
		*(uint16_t*)(kernel_setup_base + 0x212) = 0x9100; // setup_move_size
	}

	build_command_line(kernel_setup_base + COMMAND_LINE_OFFSET, 0);

	

	ext2_read_file(kernel_inode, (kernel_total_size - kernel_setup_size)/4096, kernel_setup_size/4096, 0x100000); // Read compressed kernel at 1MB

	sha256_init(&sha256ctx);
	sha256_update(&sha256ctx, 0x100000, kernel_inode->size - kernel_setup_size);
	sha256_final(&sha256ctx, hash);
	if(memcmp(hash, vmlinuz_checksum, 31)!=0){
		serial_puts("Corrupted compressed kernel image, aborting...");
		serial_putc('\n');
		return;
	}

	chacha20_xor(&chacha20ctx, 0x100000, kernel_inode->size - kernel_setup_size);

	serial_puts("Encrypted kernel loaded correctly, booting linux...");
	serial_putc('\n');



	__asm__ volatile (
		"xor %eax, %eax\n"
		"ljmp	$0x08,$0x7db6");

}

uint8_t moves[237][2]={{12, 14}, {24, 14}, {21, 9}, {17, 3}, {14, 14}, {23, 5}, {23, 2}, {23, 10}, {1, 11}, {3, 5}, {22, 6}, {27, 13}, {8, 13}, {17, 8}, {28, 12}, {1, 0}, {8, 15}, {25, 15}, {24, 3}, {13, 4}, {23, 7}, {7, 2}, {26, 14}, {11, 5}, {2, 7}, {18, 7}, {11, 4}, {11, 11}, {8, 8}, {16, 15}, {19, 15}, {13, 9}, {13, 14}, {29, 9}, {26, 12}, {25, 6}, {5, 12}, {19, 11}, {14, 11}, {22, 12}, {9, 0}, {27, 11}, {11, 9}, {8, 12}, {27, 12}, {6, 3}, {9, 8}, {1, 5}, {26, 3}, {26, 11}, {15, 12}, {29, 1}, {9, 13}, {13, 10}, {2, 9}, {24, 6}, {14, 7}, {27, 4}, {0, 6}, {19, 10}, {7, 9}, {16, 2}, {4, 9}, {15, 13}, {0, 9}, {9, 10}, {14, 0}, {0, 3}, {27, 3}, {2, 14}, {28, 13}, {25, 11}, {10, 4}, {25, 1}, {19, 7}, {14, 6}, {29, 14}, {16, 6}, {23, 3}, {6, 0}, {20, 2}, {14, 1}, {22, 9}, {1, 14}, {2, 12}, {15, 0}, {9, 11}, {6, 15}, {24, 12}, {12, 9}, {2, 4}, {13, 11}, {15, 10}, {6, 7}, {10, 3}, {4, 8}, {21, 10}, {18, 6}, {11, 2}, {21, 6}, {2, 13}, {27, 14}, {12, 5}, {26, 4}, {10, 15}, {8, 4}, {14, 15}, {9, 6}, {6, 4}, {16, 5}, {6, 5}, {12, 12}, {0, 15}, {5, 15}, {11, 0}, {28, 4}, {5, 4}, {4, 15}, {21, 8}, {10, 11}, {0, 4}, {9, 5}, {3, 11}, {25, 5}, {1, 13}, {11, 8}, {8, 5}, {12, 1}, {12, 10}, {21, 3}, {3, 4}, {11, 12}, {8, 14}, {8, 7}, {19, 6}, {8, 3}, {25, 13}, {23, 4}, {26, 0}, {11, 1}, {27, 8}, {20, 4}, {11, 13}, {13, 2}, {25, 10}, {0, 10}, {27, 15}, {10, 6}, {24, 5}, {7, 0}, {2, 15}, {15, 5}, {27, 7}, {26, 15}, {8, 2}, {24, 10}, {7, 8}, {22, 4}, {7, 10}, {25, 8}, {13, 7}, {23, 9}, {12, 15}, {26, 6}, {11, 10}, {26, 7}, {29, 15}, {29, 4}, {10, 8}, {22, 7}, {24, 4}, {10, 1}, {20, 9}, {10, 0}, {13, 0}, {28, 7}, {22, 5}, {1, 4}, {29, 6}, {23, 8}, {22, 8}, {3, 13}, {28, 15}, {10, 10}, {3, 14}, {29, 12}, {27, 6}, {18, 13}, {20, 5}, {0, 11}, {12, 3}, {11, 14}, {17, 4}, {19, 9}, {9, 4}, {26, 2}, {18, 9}, {7, 1}, {0, 5}, {8, 6}, {2, 11}, {14, 5}, {6, 10}, {15, 4}, {20, 6}, {5, 14}, {8, 10}, {19, 5}, {24, 7}, {20, 8}, {27, 5}, {10, 7}, {12, 13}, {3, 10}, {7, 11}, {12, 8}, {7, 6}, {4, 10}, {17, 15}, {0, 12}, {29, 13}, {28, 5}, {21, 4}, {8, 1}, {28, 11}, {24, 8}, {29, 3}, {1, 15}, {7, 4}, {26, 8}, {10, 14}, {11, 3}, {9, 14}, {27, 9}, {13, 13}, {12, 2}, {23, 15}};


void startSecret(int secret_inode_number){
	inode* secret_inode = ext2_inode(1, secret_inode_number);
	char* secret = ext2_read_file(secret_inode, 1, 0, NULL);
	uint8_t* queue =  malloc(960);
	int32_t queuelen = 0;
	memset(queue, '\0', 480*2);
	for(int i=0; i<480;i++){
		if(secret[i]!='\x01' && secret[i]!='\x02'){
			return;
		}
	}
	for(int i=0; i<237; i++){
		uint8_t* move = moves[i]; // For each hardcoded move
		queue[2*queuelen]=move[0]; // Add to queue
		queue[2*queuelen+1]=move[1];
		queuelen++;
		uint8_t popmove[2];
		while(queuelen>0){ // While queue is not empty
			queuelen--; // Pop one item
			popmove[0]=queue[2*queuelen];
			popmove[1]=queue[2*queuelen+1];
			uint32_t moveoffset = ((uint32_t)popmove[1])*30+(uint32_t)popmove[0]; // Access item in field
			if(secret[moveoffset]=='\x02'){ // If it's a bomb, it's a loose
				return;
			}
			secret[moveoffset]='\x03'; // Mark the spot as visited
			uint8_t neighbours[8][2] = {{popmove[0]-1,popmove[1]-1},{popmove[0]-1,popmove[1]},{popmove[0]-1,popmove[1]+1},{popmove[0],popmove[1]-1},{popmove[0],popmove[1]+1},{popmove[0]+1,popmove[1]-1},{popmove[0]+1,popmove[1]},{popmove[0]+1,popmove[1]+1}};
			uint8_t should_expand=1;
			for(int i=0; i<8 && should_expand; i++){ // Check if all neighbours aren't bombs
				if (neighbours[i][0]>=0 && neighbours[i][0]<30 && neighbours[i][1]>=0 && neighbours[i][1]<16){
					moveoffset = ((uint32_t)neighbours[i][1])*30+(uint32_t)neighbours[i][0];
					if(secret[moveoffset]=='\x02'){
						should_expand=0;
					}
				}
			}
			if(should_expand){ // If none of the neighbours are bombs
				for(int i=0; i<8; i++){
					if (neighbours[i][0]>=0 && neighbours[i][0]<30 && neighbours[i][1]>=0 && neighbours[i][1]<16){
						if(secret[((uint32_t)neighbours[i][1])*30+(uint32_t)neighbours[i][0]]=='\x01'){ // Avoid adding previously visited neighbours to the list
							queue[2*queuelen]=neighbours[i][0]; // Add neighbour to queue
							queue[2*queuelen+1]=neighbours[i][1];
							queuelen++;
						}
					}
				}
			}
		}
		
	}
	int32_t bombcount = 0;
	for(int i=0; i<480;i++){
		if(secret[i]!='\x03' && secret[i]!='\x02'){ // Ensure we visited all spots
			return;
		}
		if(secret[i]=='\x02'){
			bombcount++;
		}
	}
	if(bombcount!=99){
		return;
	}
	serial_puts("ptm bootloader unlocked successfully...");
	serial_putc('\n');
	serial_putc('\n');
	serial_puts(revflag);
	serial_putc('\n');
	serial_putc('\n');
	serial_puts("Wait, this isn't supposed to happen...");
	serial_putc('\n');
	serial_putc('\n');
	serial_puts(" --- SYSTEM HALTED --- ");
	for(;;);
}