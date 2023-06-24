# Learn_kernal_dev

## Real Mode

    1 MB of RAM is only accessible
    Memory access is done through segmentation memory model. (code segment, data segment, stack segment)

    Based on Original x86 design

    No Security for memory
    No security for hardware
    Simple user programs can destroy the operating system in real mode

    16 Bits are accessible at once
    
## Segmentation Memory Model

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