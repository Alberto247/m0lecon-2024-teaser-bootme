
#include "display.h"
__asm__(".code16gcc\n");
void BootMain(){
    clear_screen();
    print_str("Booting ptm loader 0.1...");
}