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

