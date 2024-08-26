[bits 16]
[org 0x7c00]


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
    mov dh, 20             ; dh -> num sectors
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

	xor eax, eax

	mov ax, 10h 			; Save data segment identifyer
	mov ds, ax 				; Setup data segment
	mov es, ax
	mov ss, ax				; Setup stack segment
	mov fs, ax
	mov gs, ax

	mov esp, 0x00900000		; Move temp stack pointer to 090000h

	mov eax, vid_info
	push eax
	mov eax, mem_info
	push eax
	mov edx, 0x9000
	lea eax, [edx]
	call eax				; stage2_main(mem_info, vid_info)


; boot drive variable
BOOT_DRIVE db 0

welcome db 'Booting ptm loader 0.1...', 0Ah, 0

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

gdt_end:                        ; Used to calculate the size of the GDT

gdt_desc:                       ; The GDT descriptor
	dw gdt_end - gdt - 1    ; Limit (size)
	dd gdt                  ; Address of the GDT

vid_info:
.mode	dd 0
.array 	dd 0
; padding
times 510 - ($-$$) db 0

; magic number
dw 0xaa55