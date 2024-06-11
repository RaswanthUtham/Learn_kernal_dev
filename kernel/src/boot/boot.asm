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



; ############################################### Loading the kernel into memory. #################################################
[BITS 32]
load32:
; Driver to load the kernel into memory (Driver to read data from disk since it is not advisable to use interrupt 13)
    mov eax, 1          ; load from sector 1 (which is kernel)
    mov ecx, 100        ; total sectors to load
    mov edi, 0x0100000  ; this is the address that we want to load the sectors into
    call ata_lba_read   ; Function To read the sectors into memory. Refer osdev.org ATA
    jmp CODE_SEGMENT:0x0100000  ; After loading the kernel into 0x100000 jump to that location and execute kernel.

ata_lba_read:
    ; Sending the highest 8 bits of LBA to Hard disk Controller.
    mov ebx, eax        ; backup LBA
    shr eax, 24         ; Shift eax 24 times right to send the highest 8 bits of the lba to hard disk controller
    or eax, 0xE0        ; select the master drive
    mov dx, 0x1F6       ; 0x1fx is the port in which we need to write highest 8 bits of lba.
    out dx, al          ; al register contains the higher 8 bits after the right shift which is written to port 0x1F6
    ; Finished sending the highest 8 bits of LBA

    ; send the total sectors to read
    mov eax, ecx
    mov dx, 0x1F2
    out dx, al
    ; Finished sending total sectors

    ; send all bits of the LBA
    mov eax, ebx        ; restoring the backup LBA
    mov dx, 0x1F3
    out dx, al
    ; Finished sending all bits of the LBA

    ; send remaining bits of the LBA
    mov eax, ebx        ; restoring the backup LBA
    shr eax, 8
    mov dx, 0x1F4
    out dx, al
    ; Finished sending remaining bits of the LBA

    ; send upper 16 bits of the LBA
    mov eax, ebx        ; restoring the backup LBA
    shr eax, 16
    mov dx, 0x1F5
    out dx, al
    ; Finished sending upper 16 bits of the LBA

    mov dx, 0x1F7
    mov al, 0x20
    out dx, al

    ; Read  all sectors into memory
.next_sector:
    push ecx

; Checking if we need to read again (polling)
.try_again:
    mov dx, 0x1F7
    in al, dx
    test al, 8      ; Check if bit 8 is set, if it is not set keep polling otherwise it is ready to read.
    jz .try_again

; we need to read 256 words at a time
    mov ecx, 256
    mov dx, 0x1f0
    rep insw    ; It will read the data from dx register and store it in location specified in ES:EDI (0x00:0x100000).
    pop ecx 
    loop .next_sector
; End of reading sectors into memory
    ret

times 510-($ - $$) db 0 ; Tells atleast 510 bytes to be written in memory, otherwise it will be padded with 0
dw 0xAA55  ; it will be written as 55AA in memory as intel is little endian
; 0x55AA is the signature that tells the processor that it is a boot file, which should be at the end of the boot code
