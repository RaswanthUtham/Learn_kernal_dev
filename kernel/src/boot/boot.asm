; BIOS Loads us to the address 0x7c00 in Intel
; That is why we start writing our Boot Loader from 0x7c00.

org 0x7c00

bits 16

start:

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


