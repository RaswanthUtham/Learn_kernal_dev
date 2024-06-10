; BIOS Loads us to the address 0x7c00 in Intel
; That is why we start writing our Boot Loader from 0x7c00.

org 0
bits 16

BIOS_Parameter_Block:           ; total 35 bytes  
jmp short start                 ; 2 bytes
nop
times 33 db 0                   ; setting 33 bytes of BIOS Parameter Block to 0 (TBD during File System)

handle_0:                       ; Interrupt Handler (ISR) 0  
    mov ah, 0eh
    mov al, '0'
    int 0x10
    iret

handle_1:                       ; Interrupt Handler (ISR) 1
    mov ah, 0eh
    mov al, '1'
    int 0x10
    iret


start:
    jmp 0x7c0:_start                ; Load CS register with 7c0 and jump to start

_start:
    cli                             ; Clear all interrupts
    mov ax, 0x7c0                   
    mov ds, ax                      ; Load ds with 0x7c0
    mov es, ax                      ; Load es with 0x7c0
    mov ax, 0
    mov ss, ax                      ; Load ss with 0x00 since the stack grows downwards
    mov sp, 0x7c00                  ; Load sp with 0x7c00
    sti                             ; Store all interrupts

    mov word[ss:00], handle_0        ; offset at address 00, 01 for interrupt 0
    mov word[ss:02], 0x7c0           ; segment at adress 02, 03 for interrupt 0

    mov word[ss:04], handle_1        ; offset of handle_1
    mov word[ss:06], 0x7c0           ; segment in which handle_1 resides

    int 0                           ; call interrupt 0
    int 1                           ; call interrupt 1

    mov si, message
    call print
    jmp $

print:
    mov bx, 0                   ; load_page_zero
.loop:
    lodsb                       ; load_from_si_register_to al register and_increment
    cmp al, 0
    je .done
    call print_char
    jmp .loop
.done:
    ret

print_char:
    mov ah, 0eh                 ; Tells the BIOS interrupt 10h to print a character on screen
    int 0x10                    ; call interrupt 10h
    ret

message: db 'Hello World', 0

times 510 - ($ - $$) db 0       ; Fill remaining bytes with 0 (total 510 bytes needed)
dw 0xAA55                       ; Boot Loader End Signature in Little Endian Format


