; BIOS Loads us to the address 0x7c00 in Intel
; That is why we start writing our Boot Loader from 0x7c00.

; ############################ Real Mode ######################################
org 0x7c00
bits 16

CODE_SEGMENT equ gdt_code - gdt_start
DATA_SEGMENT equ gdt_data - gdt_start

BIOS_Parameter_Block:           ; total 35 bytes  
jmp short start                 ; 2 bytes
nop
times 33 db 0                   ; setting 33 bytes of BIOS Parameter Block to 0 (TBD during File System)

; ########################### Load Segment Registers ################################
start:
    jmp 0:_start                ; Load CS register with 7c0 and jump to start

_start:
    cli                             ; Clear all interrupts
    mov ax, 0                   
    mov ds, ax                      ; Load ds with 0x7c0
    mov es, ax                      ; Load es with 0x7c0
    mov ss, ax                      ; Load ss with 0x00 since the stack grows downwards
    mov sp, 0x7c00                  ; Load sp with 0x7c00
    sti                             ; Store all interrupts

; ########################### End Load Segment Registers ################################
; ############################ End Real Mode #############################################



; ############################ Protected Mode #############################################
    cli
    lgdt[gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SEGMENT:load32

; ################################# GDT Structure ###################################################
; Global Descriptor Table (Contains the information about segments and its privelage levels)
gdt_start:
gdt_null: ; writing 1st 8 bytes (0 to 64 bits) of GDT to zero
    dd 0
    dd 0

; offset 0x08 (Next 8 bytes)
gdt_code: ; CS should point to this
    dw 0xffff    ;Segment limit 0 - 15 bits
    dw 0        ;Base first 0-15 bits
    db 0        ; Base 16-23 bits
    db 9ah      ; Access byte
    db 11001111b ; High 4 and low 4 bit flage
    db 0        ; Base 24-31 bits


; offset 0x10 (Next 8 bytes)
gdt_data: ; DS, SS, ES, FS, GS
    dw 0xffff    ;Segment limit 0 - 15 bits
    dw 0        ;Base first 0-15 bits
    db 0        ; Base 16-23 bits
    db 92h      ; Access byte
    db 11001111b ; High 4 and low 4 bit flage
    db 0        ; Base 24-31 bits
gdt_end:

gdt_descriptor: ; contains size and start address of GDT table
    gdt_size            dw gdt_end - gdt_start - 1 ; size
    gdt_start_address   dd gdt_start ; address

bits 32
load32:
    mov ax, DATA_SEGMENT
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp

; Enable A20 line (21st bit of address line)
    in al, 0x92
    or al, 2
    out 0x92, al
    
    jmp $
    
times 510 - ($ - $$) db 0       ; Fill remaining bytes with 0 (total 510 bytes needed)
dw 0xAA55                       ; Boot Loader End Signature in Little Endian Format


