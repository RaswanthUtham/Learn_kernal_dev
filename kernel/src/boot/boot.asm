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
    mov ebx, eax        ;  Copies the value in eax (which contains the LBA) into ebx to preserve the original LBA value for later use.
    shr eax, 24         ; Performs a logical right shift on eax by 24 bits. This isolates the highest 8 bits of the LBA and places them in the lower 8 bits of eax.
    or eax, 0xE0        ; Sets the highest 4 bits of eax to 1110 (binary) to select the master drive and mark the command as LBA addressing.
    mov dx, 0x1F6       ; Sets the dx register to the port address 0x1F6, which is used for the highest 8 bits of the LBA.
    out dx, al          ; Sends the lowest 8 bits of eax (which now contain the highest 8 bits of the LBA) to the port 0x1F6.
    ; Finished sending the highest 8 bits of LBA

    ; send the total sectors to read
    mov eax, ecx ; Copies the value in ecx (which contains the number of sectors to read) into eax.
    mov dx, 0x1F2; Sets the dx register to the port address 0x1F2, which is used to specify the number of sectors to read.
    out dx, al   ; Sends the lowest 8 bits of eax (which contain the sector count) to the port 0x1F2.
    ; Finished sending total sectors

    ; send all bits of the LBA
    mov eax, ebx        ; Restores the original LBA value from ebx back into eax.
    mov dx, 0x1F3       ; Sets the dx register to the port address 0x1F3, which is used for the lowest 8 bits of the LBA.
    out dx, al          ; Sends the lowest 8 bits of eax (which contain the lowest 8 bits of the LBA) to the port 0x1F3
    ; Finished sending all bits of the LBA

    ; send remaining bits of the LBA
    mov eax, ebx        ; Restores the original LBA value from ebx back into eax.
    shr eax, 8          ; Performs a logical right shift on eax by 8 bits. This places the second lowest 8 bits of the LBA in the lower 8 bits of eax.
    mov dx, 0x1F4       ; Sets the dx register to the port address 0x1F4, which is used for the next 8 bits of the LBA.
    out dx, al          ; Sends the lowest 8 bits of eax (which contain the next 8 bits of the LBA) to the port 0x1F4.
    ; Finished sending remaining bits of the LBA

    ; send upper 16 bits of the LBA
    mov eax, ebx        ; Restores the original LBA value from ebx back into eax
    shr eax, 16         ; Performs a logical right shift on eax by 16 bits. This places the third lowest 8 bits of the LBA in the lower 8 bits of eax.
    mov dx, 0x1F5       ; Sets the dx register to the port address 0x1F5, which is used for the upper 8 bits of the LBA.
    out dx, al          ; Sends the lowest 8 bits of eax (which contain the upper 8 bits of the LBA) to the port 0x1F5.
    ; Finished sending upper 16 bits of the LBA

    mov dx, 0x1F7       ; Sets the dx register to the port address 0x1F7, which is used for sending commands to the disk.
    mov al, 0x20        ; Places the value 0x20 (the command for reading sectors) into the al register.
    out dx, al          ; Sends the command 0x20 to the port 0x1F7 to initiate the read operation.

    ; Read  all sectors into memory
.next_sector:
    push ecx            ;  Pushes the value in ecx (sector count) onto the stack to preserve it for later use.

; Checking if we need to read again (polling)
.try_again:
    mov dx, 0x1F7       ; Sets the dx register to the port address 0x1F7, which is used for reading the status of the disk.
    in al, dx           ; Reads the value from the status port 0x1F7 into the al register
    test al, 8          ; Tests if the third bit (DRQ Data Request) in al is set. The test instruction performs a bitwise AND and sets the zero flag based on the result.
    jz .try_again       ; If the zero flag is set (indicating that the DRQ bit is not set), jumps back to .try_again to continue polling.

; we need to read 256 words at a time
    mov ecx, 256        ; Sets ecx to 256, indicating that 256 words (512 bytes) need to be read
    mov dx, 0x1f0       ; Sets the dx register to the port address 0x1F0, which is used for reading data from the disk. 
    rep insw            ; The rep prefix repeats the insw instruction (which reads a word from the dx port into the memory location pointed to by es:edi) 256 times. It will read the data from dx register and store it in location specified in ES:EDI (0x00:0x100000).
    pop ecx             ; Pops the saved sector count from the stack back into ecx.
    loop .next_sector   ; Decrements ecx and jumps to .next_sector if ecx is not zero, repeating the process for the next sector if necessary.
; End of reading sectors into memory
    ret

; This code sets up the ATA controller to read a specified number of sectors starting from a given LBA, then reads the data from the disk into memory, sector by sector. The polling mechanism ensures that the disk is ready to transfer data before each read operation.

times 510-($ - $$) db 0 ; Tells atleast 510 bytes to be written in memory, otherwise it will be padded with 0
dw 0xAA55  ; it will be written as 55AA in memory as intel is little endian
; 0x55AA is the signature that tells the processor that it is a boot file, which should be at the end of the boot code
