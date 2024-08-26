#include "defs.h"
#include "crunch.h"
#include "ext2.h"

uint32_t HEAP = 	0x00200000;
uint32_t HEAP_START;

static void putpixel(unsigned char* screen, int x,int y, int color);
void parse_config(char* config);

void stage2_main(uint32_t* mem_info, vid_info* v) {
	//clear the screen
	HEAP_START = HEAP;
	vga_clear();
	vga_pretty("Stage2 loaded...\n", VGA_LIGHTGREEN);
	lsroot();

	mmap* m = (mmap*) *mem_info;
	mmap* max = (mmap*) *++mem_info;
	printx("Video Mode:", v->mode);
	vga_puts("framebuffer@ 0x");

	vga_puts(itoa(v->info->framebuffer, 16));
	vga_putc('\n');
	/* Don't use the heap, because it's going to be wiped */
	// gfx_context *c = (gfx_context*) 0x000F0000;
	// c->pitch = v->info->pitch;
	// c->width = v->info->width;
	// c->height = v->info->height;
	// c->bpp = v->info->bpp;
	// c->framebuffer = v->info->framebuffer;
	// memcpy(0x000F1000, m, (uint32_t) max - (uint32_t) m);

	// vga_pretty("Memory map:\n", VGA_LIGHTGREEN);
	// while (m < max) {

	// 	vga_puts(itoa(m->base, 16));
	// 	vga_putc('-');
	// 	vga_puts(itoa(m->len + m->base, 16));
	// 	vga_putc('\t');
	// 	//vga_puts(itoa(m->type, 10));
	// 	switch((char)m->type) {
	// 		case 1: {
	// 			vga_puts("Usable Ram\t");

	// 			break;
	// 		} case 2: {
	// 			vga_puts("Reserved\t");
	// 			break;
	// 		} default:
	// 			vga_putc('\n');
	// 	}
	// 			vga_puts(itoa(m->len/0x400, 10));
	// 			vga_puts(" KB\n");
	// 	m++;
	// }

	int kernel_inode_number = ext2_find_child("bzImage", 2);

	printx("kernel inode:", kernel_inode_number);

	inode* kernel_inode = ext2_inode(1, kernel_inode_number);

	char* kernel = ext2_read_file(kernel_inode);

	unsigned int kernel_setup_size = ((*(int8_t*)(kernel+0x1f1))+1)<<9;

	unsigned int kernel_total_size = (kernel_inode->blocks / (4096/SECTOR_SIZE))*4096;

	unsigned int kernel_hook = (*(int32_t*)(kernel+0x214));
	
	printx("kernel size:", kernel_total_size);
	printx("kernel magic:", *(int16_t*)(kernel+0x01fe));
	printx("kernel setup size:", kernel_setup_size);
	printx("kernel hook:", kernel_hook);

	memcpy(0x100000, kernel+kernel_setup_size, kernel_total_size-kernel_setup_size);

	vga_puts("Copied data");
	vga_putc('\n');

	((void(*)(void))kernel_hook)();

	/* We should never reach this point */
	for(;;);
}

void parse_config(char* config) {
	vga_puts(config);
	

}

static void putpixel(unsigned char* screen, int x,int y, int color) {
	unsigned where = x*3 + y*768*4;
	screen[where] = color & 255;              // BLUE
	screen[where + 1] = (color >> 8) & 255;   // GREEN
	screen[where + 2] = (color >> 16) & 255;  // RED
}
