# Interrupt Descriptor table

The Interrupt Descriptor Table (IDT) is a crucial data structure used in x86 and x86-64 architectures to define the response to various interrupts and exceptions. Each entry in the IDT is called an Interrupt Descriptor, and it specifies the address of the interrupt handler along with other control information. Here are the fields typically found in an IDT entry:

## Structure of an IDT Entry

An IDT entry is usually 8 bytes (64 bits) in real mode and 16 bytes (128 bits) in protected mode on x86 architectures. Here's a breakdown of the fields in each entry:

### 32-bit Protected Mode IDT Entry (8 bytes)

1. Offset (15:0): The lower 16 bits of the handler function's address.
2. Selector: A 16-bit value that specifies the segment selector for the code segment containing the handler. This is typically the kernel code segment.
3. Reserved: An 8-bit field that is reserved and should be set to zero.
4. Type and Attributes: An 8-bit field that includes several sub-fields:

        Type: Specifies the type of gate (e.g., 32-bit interrupt gate, trap gate, task gate).
        DPL (Descriptor Privilege Level): Specifies the privilege level required to access the interrupt. Values range from 0 (highest privilege) to 3 (lowest privilege).
        P (Present): A 1-bit flag indicating whether the interrupt descriptor is present in memory.

5. Offset (31:16): The upper 16 bits of the handler function's address.

### 64-bit Long Mode IDT Entry (16 bytes)
1. Offset (15:0): The lower 16 bits of the handler function's address.
2. Selector: A 16-bit value that specifies the segment selector for the code segment containing the handler.
3. IST (Interrupt Stack Table): A 3-bit field specifying the stack to be used for the interrupt.
4. Reserved: A 5-bit field that is reserved and should be set to zero.
5. Type and Attributes: An 8-bit field similar to the one in 32-bit mode.
6. Offset (31:16): The next 16 bits of the handler function's address.
7. Offset (63:32): The upper 32 bits of the handler function's address.
8. Reserved: A 32-bit field that is reserved and should be set to zero.

## Detailed Explanation of Fields
1. Offset: The address of the interrupt handler. In 32-bit mode, it's split into two parts: lower and upper 16 bits. In 64-bit mode, it's split into three parts to cover the full 64-bit address space.
2. Selector: This segment selector points to a segment descriptor in the GDT (Global Descriptor Table) or LDT (Local Descriptor Table) that contains the code for the interrupt handler.
3. IST: In 64-bit mode, this specifies which stack to use from the Interrupt Stack Table. It's useful for handling interrupts that require a different stack (e.g., double faults).
4. Reserved: Fields reserved for future use or alignment purposes, usually set to zero.
5. Type and Attributes:

        . Type: Defines the type of gate (interrupt gate, trap gate, or task gate).
        . DPL: The privilege level required to invoke this interrupt.
        . P: Indicates whether the descriptor is present and valid.


## Gate Types

1. Interrupt Gate

    Purpose: Used for handling hardware and software interrupts.
    Characteristics:
        Automatically clears the Interrupt Flag (IF), disabling further interrupts while the current interrupt is being processed. This prevents other interrupts from interfering with the handler.
        After handling the interrupt, the IF flag is restored to its previous state.
    Usage: Commonly used for regular hardware interrupts (e.g., keyboard or timer interrupts).
    Types:
        32-bit interrupt gate (for protected mode).
        64-bit interrupt gate (for long mode).

2. Trap Gate

    Purpose: Used for handling exceptions and software interrupts that should not disable further interrupts.
    Characteristics:
        Does not clear the Interrupt Flag (IF), allowing other interrupts to be serviced while handling the current one.
        Useful for debugging and other scenarios where interrupt nesting is needed.
    Usage: Commonly used for exceptions like divide-by-zero or page faults, and for debugging traps.
    Types:
        32-bit trap gate (for protected mode).
        64-bit trap gate (for long mode).


3. Task Gate

    Purpose: Used to switch to a different task when an interrupt or exception occurs.
    Characteristics:
        Does not transfer control directly to an interrupt handler. Instead, it triggers a task switch by loading a new Task State Segment (TSS).
        Facilitates multitasking by allowing an interrupt to initiate a context switch to a different task.
    Usage: Rarely used in modern operating systems but can be used for implementing task switching in systems that use hardware task switching.
    Types:
        32-bit task gate (for protected mode).

## Detailed Field Explanation for Each Gate

Interrupt Gate and Trap Gate

    Offset (15:0): Lower 16 bits of the handler's address.
    Selector: Segment selector for the handler's code segment.
    IST (Interrupt Stack Table): In 64-bit mode, specifies which stack to use.
    Reserved: Reserved bits, usually set to zero.
    Type and Attributes: Defines the gate type (interrupt/trap), DPL, and present bit.
    Offset (31:16): Upper 16 bits of the handler's address.
    Offset (63:32): In 64-bit mode, the next 32 bits of the handler's address.
    Reserved: Additional reserved bits, usually set to zero.

Task Gate

    Selector: Segment selector for the Task State Segment (TSS) that should be used.
    Reserved: Additional reserved bits, usually set to zero.
    Type and Attributes: Defines the gate type (task gate), DPL, and present bit.
    Reserved: Additional reserved bits, usually set to zero.

Summary

    Interrupt Gate: Handles hardware and software interrupts, disables further interrupts by clearing the IF flag.
    Trap Gate: Handles exceptions and allows further interrupts by not clearing the IF flag.
    Task Gate: Switches to a different task by loading a new TSS, used for hardware task switching.
