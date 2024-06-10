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
