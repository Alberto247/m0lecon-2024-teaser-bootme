#include "defs.h"

/*
Disclaimer:

Yes, lo so che si chiama vga.c e implementa una seriale
Prima implementava vga, poi mi sono reso conto che non era comodo ma 0 sbatti di cambiare il nome
*/


void printx(char* msg, uint32_t i) {
	serial_puts(msg);
	serial_puts(itoa(i, 16));
	serial_putc('\n');
}

#define PORT 0x3f8          // COM1
 
int init_serial() {
   outb(PORT + 1, 0x00);    // Disable all interrupts
   outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   outb(PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
   outb(PORT + 1, 0x00);    //                  (hi byte)
   outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
   outb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
   outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
   outb(PORT + 4, 0x1E);    // Set in loopback mode, test the serial chip
   outb(PORT + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)
 
   // Check if serial is faulty (i.e: not same byte as sent)
   if(inb(PORT + 0) != 0xAE) {
      return 1;
   }
 
   // If serial is not faulty set it in normal operation mode
   // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
   outb(PORT + 4, 0x0F);
   return 0;
}

int is_transmit_empty() {
   return inb(PORT + 5) & 0x20;
}

void serial_putc(char a) {
   while (is_transmit_empty() == 0);
   outb(PORT,a);
}

void serial_puts(char* s){
	int i = 0;
	while (*s != 0) {
		serial_putc(*s);
		*s++;
	}
}

void print_hash(uint8_t hash[]){
	serial_puts(itoa(((uint32_t*)hash)[0], 16));
	serial_puts(itoa(((uint32_t*)hash)[1], 16));
	serial_puts(itoa(((uint32_t*)hash)[2], 16));
	serial_puts(itoa(((uint32_t*)hash)[3], 16));
	serial_puts(itoa(((uint32_t*)hash)[4], 16));
	serial_puts(itoa(((uint32_t*)hash)[5], 16));
	serial_puts(itoa(((uint32_t*)hash)[6], 16));
	serial_puts(itoa(((uint32_t*)hash)[7], 16));
}