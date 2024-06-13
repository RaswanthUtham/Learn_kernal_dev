# Programmable Interrupt Controller

A Programmable Interrupt Controller (PIC) is a device used in computer systems to manage hardware interrupts and send them to the appropriate processor. The PIC allows the CPU to prioritize interrupts and handle multiple interrupt requests from different sources efficiently.

## Key Functions and Features of a PIC

        Interrupt Prioritization:
            The PIC prioritizes interrupt requests (IRQs) to ensure that higher-priority interrupts are handled before lower-priority ones.

        Interrupt Masking:
            The PIC can enable or disable specific interrupts using interrupt masks, allowing the system to ignore certain interrupts if needed.

        Interrupt Vectoring:
            The PIC provides the CPU with the interrupt vector number, which points to the appropriate interrupt handler in the Interrupt Descriptor Table (IDT).

        Chaining and Cascading:
            Multiple PICs can be chained or cascaded to handle more interrupt lines than a single PIC can manage. This is common in systems with a large number of devices generating interrupts.

## Types of Programmable Interrupt Controllers

### 8259A PIC:
        Overview: The Intel 8259A is one of the most well-known PICs. It is used in many older PC architectures and can manage up to 8 interrupt lines.
        Cascading: Two 8259A PICs can be cascaded to handle up to 15 IRQs (one IRQ is used for cascading).
        Compatibility: The 8259A is designed for compatibility with the x86 architecture.

### APIC (Advanced Programmable Interrupt Controller):
        Overview: The APIC is used in modern multiprocessor systems to handle a higher number of interrupts and provide better performance and flexibility than the 8259A.
        Features:
            Local APIC: Each processor has a Local APIC that handles interrupts for that specific processor.
            I/O APIC: An I/O APIC handles interrupts from peripheral devices and can route them to any processor in the system.
        Scalability: APICs can handle more interrupts and support advanced features like interrupt routing, interrupt priority levels, and more efficient handling of inter-processor interrupts (IPIs).

## Operation of a PIC

    1. Interrupt Request:
        When a hardware device needs attention, it sends an interrupt request (IRQ) to the PIC.

    2. Prioritization and Masking:
        The PIC prioritizes the interrupt request based on its priority level and checks the interrupt mask to determine if the interrupt should be forwarded to the CPU.

    3. Interrupt Vectoring:
        If the interrupt is not masked, the PIC sends an interrupt vector number to the CPU, which points to the corresponding entry in the IDT.

    4. Interrupt Acknowledgment:
        The CPU acknowledges the interrupt and executes the interrupt handler routine specified by the IDT entry.

    5. End of Interrupt (EOI):
        Once the interrupt handler completes, it sends an End of Interrupt (EOI) signal to the PIC, allowing the PIC to process the next pending interrupt.

## Summary

    Programmable Interrupt Controller (PIC): A device that manages hardware interrupts, prioritizes them, and sends them to the CPU.
    Key Functions: Interrupt prioritization, masking, and vectoring.
    Types:
        8259A PIC: Common in older systems, handles up to 8 interrupts, can be cascaded.
        APIC: Used in modern multiprocessor systems, includes Local and I/O APICs, offers advanced features.
    Operation: Handles interrupt requests, prioritizes and masks them, vectors interrupts to the CPU, and manages interrupt acknowledgment and completion.

The PIC is essential for efficient interrupt handling in computer systems, ensuring that the CPU can manage multiple interrupt sources effectively.

# Implemented in kernel.asm

1. Initialize the PIC
2. remap the interrupt numbers to irq number in pic (irq is remapped to 0x20)
3. keep the pic in x86 mode


