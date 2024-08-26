extern  BootMain
[bits 16]

section .text
start:
    cli
    mov ax, cs
    mov ds,ax               ; Make DS correct
    mov es,ax               ; Make ES correct
    mov ss,ax
    sti
    call BootMain
    jmp $


