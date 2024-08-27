#include "defs.h"
#include "crunch.h"
#include "ext2.h"

uint32_t HEAP = 	0x00200000;
uint32_t HEAP_START;

#define COMMAND_LINE_OFFSET 0x9000

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
	unsigned int kernel_total_size = (kernel_inode->blocks / (4096/SECTOR_SIZE))*4096;
	startbzImage(kernel, kernel_total_size);
	
	vga_puts("Error");
	vga_putc('\n');

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
	//char *env_command_line;

	command_line[0] = '\0';

	strcat(command_line, "console=ttyS0 vga=normal mem=4G auto");

	

	// env_command_line =  getenv("bootargs");

	// /* set console= argument if we use a serial console */
	// if (NULL == strstr(env_command_line, "console=")) {
	// 	if (0==strcmp(getenv("stdout"), "serial")) {

	// 		/* We seem to use serial console */
	// 		sprintf(command_line, "console=ttyS0,%s ",
	// 			 getenv("baudrate"));
	// 	}
	// }

	// if (auto_boot) {
	// 	strcat(command_line, "auto ");
	// }

	// if (NULL != env_command_line) {
	// 	strcat(command_line, env_command_line);
	// }


	//printf("Kernel command line: \"%s\"\n", command_line);
}



// TODO: sanity checks on kernel size!!!
void startbzImage(char* kernel, unsigned int kernel_total_size){
	if((int16_t)(*(int16_t*)(kernel+0x01fe))!=(int16_t)0xaa55){
		vga_puts("Invalid kernel magic.");
		printx("kernel magic:", *(int16_t*)(kernel+0x01fe));
		return;
	}
	if(*(int32_t*)(kernel+0x202)!=0x53726448){
		vga_puts("Only V2 kernel is supported.");
		return;
	}

	unsigned int kernel_setup_base=0x90000;

	uint16_t boot_proto = *(uint16_t*)(kernel+0x206);
	unsigned int kernel_setup_size = ((*(int8_t*)(kernel+0x1f1))+1)<<9;

	unsigned int kernel_hook = (*(int32_t*)(kernel+0x214));
	
	printx("kernel size:", kernel_total_size);
	
	printx("kernel setup size:", kernel_setup_size);
	printx("kernel hook:", kernel_hook);

	memcpy(kernel_setup_base, kernel, kernel_setup_size);

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


	memcpy(0x100000, kernel+kernel_setup_size, kernel_total_size-kernel_setup_size);

	vga_puts("Copied data");
	vga_putc('\n');

	__asm__ volatile (
		"xor %eax, %eax\n"
		"ljmp	$0x08,$0x7dcc");

}