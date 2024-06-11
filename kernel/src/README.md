# Protected Mode

## Memory And Hardware Protection

    Allows the user program from accessing the memory and hardware directly.

        Ring 0: Kernel is present. It is the Most Privelaged ring
        Ring 1: Device Drivers
        Ring 2: Device Drivers
        Ring 3: User Applications. Least privelaged ring

## Memory Schemes:

    Selectors: 
        Segment registers will be called as Selector registers. (Code Selector, Data Selector, Stack Selector, ...)
        Selectors point to data structures called Global Descriptor Table (GDT), that describes the memory ranges and permissions (ring levels) required to access the memory

    Paging: 
        Remapping Physical Memory to Virtual Memory

    4gb of memory can be accessed

## Loading the kernel into RAM

### boot.asm
1. start bootloader in 0x7c00 ram address
2. write bios parameter block, which gives information about the filesystem to bootloader
3. load the segment registers with 0 since we start the origin from 0x7c00
4. define the global descriptor table which has information about the segments and the privelege levels ring 0 to ring 4
5. Enter into protected mode
6. Now segment registers become selector registers because of gdt
7. write a driver to read data from disk.
8. That driver (ata_lba_read) will read the kernel (kernel.asm) from disk (os.bin) and load it in RAM location 1MB.
9. Check linker.ld and Makefile

### kernel.asm
1. Load the selector registers
2. Enable the a20 address line

### linker.ld
1. Kernel is loaded at 1MB RAM location which is defined here

### Makefile
1. boot.asm to boot.bin
2. kernel.asm and other_kernel_files to kernel.asm.o and other_kernel_files.o
3. kerne;.asm.o and other_kernel_files.o to kernelfull.o
4. kernelfull.o to kernel.bin
5. boot.bin + kernel.bin to os.bin


