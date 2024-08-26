[bits 16]
[org 0x7c00]

; where to load the kernel to
STAGE2_OFFSET equ 0x1000

start:
    ; BIOS sets boot drive in 'dl'; store for later use
    mov [BOOT_DRIVE], dl

    ; setup stack
    mov bp, 0x9000
    mov sp, bp


    mov ah, 0x0e
    lea si, [welcome]
    loop:
        lodsb
        test al, al
        jz endwrite
        int 0x10
        jmp loop

    endwrite:
    call load_stage2
    call switch_to_stage2

    jmp $

disk_load:
    pusha
    push dx

    mov ah, 0x02 ; read mode
    mov al, dh   ; read dh number of sectors
    mov cl, 0x02 ; start from sector 2
                 ; (as sector 1 is our boot sector)
    mov ch, 0x00 ; cylinder 0
    mov dh, 0x00 ; head 0

    ; dl = drive number is set as input to disk_load
    ; es:bx = buffer pointer is set as input as well

    int 0x13      ; BIOS interrupt
    jc disk_error ; check carry bit for error

    pop dx     ; get back original number of sectors to read
    cmp al, dh ; BIOS sets 'al' to the # of sectors actually read
               ; compare it to 'dh' and error out if they are !=
    jne sectors_error
    popa
    ret

disk_error:
    jmp disk_loop

sectors_error:
    jmp disk_loop

disk_loop:
    jmp $

load_stage2:
    mov bx, STAGE2_OFFSET ; bx -> destination
    mov dh, 20             ; dh -> num sectors
    mov dl, [BOOT_DRIVE]  ; dl -> disk
    call disk_load
    ret

switch_to_stage2:
    call STAGE2_OFFSET ; give control to the kernel
    jmp $ ; loop in case kernel returns

write_char:
    mov ah, 0x0e ; char written in al
    int 0x10

; boot drive variable
BOOT_DRIVE db 0

welcome db 'Booting ptm loader 0.1...', 0Ah, 0

; padding
times 510 - ($-$$) db 0

; magic number
dw 0xaa55