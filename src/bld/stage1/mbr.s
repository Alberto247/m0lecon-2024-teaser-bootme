[bits 16]
[org 0x7c00]


start:
    ; BIOS sets boot drive in 'dl'; store for later use
    mov [BOOT_DRIVE], dl

    ; setup stack
    mov bp, 0x9000
    mov sp, bp
    
    call load_stage2
    call video_map
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
    mov bx, 0x9000 ; bx -> destination
    mov dh, 40             ; dh -> num sectors
    mov dl, [BOOT_DRIVE]  ; dl -> disk
    call disk_load
    ret

video_map:
	xor eax, eax
	mov es, ax	
	mov bx, 0x2000
	mov di, bx
	mov ax, 0x4F00
	int 0x10

	cmp ax, 0x004F
	jne .error

	;mov [vid_info+4], bx		; Store pointer to video controller array
	mov si, [bx + 0xE]		; Offset to mode pointer
	mov ax, [bx + 0x10]		; Segment to mode pointer
	mov es, ax

	mov di, 0x3000

.loop:
	mov bx, [es:si]			; Load BX with video mode
	cmp bx, 0xFFFF
	je .done				; End of list

	add si, 2
	mov [.mode], bx

	mov ax, 0x4F01			; Get mode info pitch+16, width+18, height+20
	mov cx, [.mode]

	int 0x10
	cmp ax, 0x004F
	jne .error
	xor ax, ax

	mov ax, [es:di + 42]
	mov [.framebuffer], ax

	mov ax, [es:di + 16]	; pitch
	mov bx, [es:di + 18]	; width
	mov cx, [es:di + 20]	; height
	mov dx, [es:di + 25]	; bpp
	mov [.bpp], dx

	add di, 0x100

	cmp ax, [.pitch]
	jne .loop

	cmp bx, [.width]
	jne .loop

	cmp cx, [.height]
	jne .loop

	lea ax, [es:di - 0x100]
	mov [vid_info.array], ax	; Pointer to mode array

.setmode:
	xor ax, ax
	mov ax, 0x4F02		; Function AX=4F02h;
	mov bx, [.mode]
	mov [vid_info], bx
	or bx, 0x4000		; enable LFB

	; int 0x10
	; cmp ax, 0x004F
	; jne .error

.done:
	ret

.error:
	jmp $

.mode 			dw 0
.width 			dw 1024
.height 		dw 768
.pitch 			dw 3072
.bpp			dw 0
.framebuffer	dd 0

switch_to_stage2:
    ; enter protected mode
	cli 				; Turn off interrupts
	;xor ax, ax
	in al, 0x92			; enable a20
	or al, 2
	out 0x92, al

	xor ax, ax 			; Clear AX register
	mov ds, ax			; Set DS-register to 0 
	mov es, ax
	mov fs, ax
	mov gs, ax

	lgdt [gdt_desc] 	; Load the Global Descriptor Table

;==============================================================================
; ENTER PROTECTED MODE 

	mov eax, cr0
	or eax, 1               ; Set bit 0 
	mov cr0, eax

	jmp 08h:pm 				; Jump to code segment, offset kernel_segments


[BITS 32]
	pm:
	pop ecx					; End of memory map
	mov [mem_info+4], ecx 
	pop ecx
	mov [mem_info], ecx

	mov eax, 0x10000
	mov edx, 0x9000
	mov ebx, 0x5000
	copyloopstart:
	mov ecx, [edx]
	mov [eax], ecx
	add eax, 4
	add edx, 4
	sub ebx, 4
	cmp ebx, 0
	jg copyloopstart

	xor eax, eax

	mov ax, 10h 			; Save data segment identifyer
	mov ds, ax 				; Setup data segment
	mov es, ax
	mov ss, ax				; Setup stack segment
	mov fs, ax
	mov gs, ax

	mov esp, 0x00f00000		; Move temp stack pointer to 0f0000h

	mov eax, vid_info
	push eax
	mov eax, mem_info
	push eax
	mov edx, 0x10000
	lea eax, [edx]
	call eax				; stage2_main(mem_info, vid_info)


; boot drive variable
BOOT_DRIVE db 0

;==============================================================================
; GLOBAL DESCRIPTOR TABLE

align 16
mem_info:
	dd 0			
	dd 0



align 32
gdt:                            ; Address for the GDT

gdt_null:
	dd 0
	dd 0

;KERNEL_CODE equ $-gdt		; 0x08
gdt_kernel_code:
	dw 0FFFFh 	; Limit 0xFFFF
	dw 0		; Base 0:15
	db 0		; Base 16:23
	db 09Ah 	; Present, Ring 0, Code, Non-conforming, Readable

	db 0CFh		; Page-granular
	db 0 		; Base 24:31

;KERNEL_DATA equ $-gdt
gdt_kernel_data:                        
	dw 0FFFFh 	; Limit 0xFFFF
	dw 0		; Base 0:15
	db 0		; Base 16:23
	db 092h 	; Present, Ring 0, Code, Non-conforming, Readable
	db 0CFh		; Page-granular
	db 0 		; Base 24:31

gdt16_code:
    ; 16-bit 4gb flat r/w/executable code descriptor
    dw 0xFFFF                   ; limit low
    dw 0                        ; base low
    db 0                        ; base middle
    db 10011010b                ; access
    db 10001111b                ; granularity
    db 0                        ; base high

gdt16_data:
    ; 16-bit 4gb flat r/w data descriptor
    dw 0xFFFF                   ; limit low
    dw 0                        ; base low
    db 0                        ; base middle
    db 10010010b                ; access
    db 10001111b                ; granularity
    db 0                        ; base high

gdt_end:                        ; Used to calculate the size of the GDT

gdt_desc:                       ; The GDT descriptor
	dw gdt_end - gdt - 1    ; Limit (size)
	dd gdt                  ; Address of the GDT

vid_info:
.mode	dd 0
.array 	dd 0
; padding

times 438 - ($-$$) db 0



[bits 32]

realmodejump:
	mov eax, 20h
	mov ds, eax
	mov es, eax
	mov fs, eax
	mov gs, eax
    jmp 0x18:realmodeenter

[bits 16]
realmodeenter:
	cli
	mov eax, cr0
	and eax, 0x7ffffffe
	mov cr0, eax
	jmp 0x0:kernelstart

;[bits 16]
kernelstart:
    mov ax, 0x9000
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov sp, 0xe000
	jmp 9020h:0

times 510 - ($-$$) db 0

; magic number
dw 0xaa55