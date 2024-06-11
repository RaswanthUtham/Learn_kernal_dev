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


