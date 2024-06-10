# Kernel Development

1. The CPU executes instructions directly from BIOS from ROM
2. The BIOS generally loads itself into RAM and continues execution from RAM
3. The BIOS will initialise essential hardware
4. The BIOS looks for a bootloader to boot by searching all storage mediums for the boot signature 0x55aa
5. The BIOS loads the bootloader into RAM at absolute address 0x7c00
6. The BIOS instructs the process to jump to 0x7c00 and begin executing the operating system bootloader

# Real mode

1. 1 MB of RAM is only accessible
2. Memory access is done through segmentation memory model. (code segment, data segment, stack segment)
 
3. Based on Original x86 design
 
4. No Security for memory
5. No security for hardware
6. Simple user programs can destroy the operating system in real mode
 
7. 36 Bits are accessible at once

# Segmentation Memory Model

    Memory is accessed by segment and offset
    Programs can be loaded into different areas of memory without any problems
    Multiple segments are available through the use of segment registers

    Segment Registers:
        CS: Code Segment
        DS: Data Segment
        SS: Stack Segment
        ES: Extra Segment
    These registers will help us to access different memory segments in RAM

    Offset Calculation:
        Take the segment address, multiply it with 16 and add the offset
        example: segment address is 0x7c0 and offset is 0x21
        then 0x7c0 * 16 = 0x7c00
        0x7c00 = 0x21 = 0x7c21 is the actual address.

    Different Instructions use different segment register

    Example:
    lodsb uses DS:SI
    Data Segment and SI register as offset

# BIOS Parameter Block

    The BIOS Parameter Block (BPB) is a data structure found in the Master Boot Record (MBR) of a storage device, such as a hard drive or a USB flash drive. It contains important information that the BIOS (Basic Input/Output System) uses to boot the computer and access the file system on the storage device.

    The BPB includes details such as the sector size, number of sectors per cluster, number of reserved sectors, number of file allocation tables (FATs), and other parameters related to the file system. These parameters help the BIOS understand the layout of the storage device and locate the bootloader, which is responsible for loading the operating system.

    The BPB is specific to older BIOS-based systems and the traditional FAT file systems (FAT12, FAT16, FAT32). Modern systems that use the Unified Extensible Firmware Interface (UEFI) instead of BIOS typically have a different data structure called the GUID Partition Table (GPT) for storing similar information.

    In summary, the BIOS Parameter Block is a data structure within the MBR that provides vital information to the BIOS for booting and accessing the file system on a storage device.
