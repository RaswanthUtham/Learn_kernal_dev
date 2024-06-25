## FAT formatted disk organisation

Here's a simplified view of how a FAT-formatted disk might be organized:

1. Boot Sector: Contains the BIOS Parameter Block (BPB) and boot code.
2. Reserved Sectors: Reserved for the boot sector and possibly other uses (e.g., space for additional boot code).
3. FAT #1: The first File Allocation Table.
4. FAT #2: The second File Allocation Table (if a second copy is used for redundancy).
5. Root Directory: Contains directory entries for files and subdirectories in the root directory.
6. Data Area: The area where the actual file and directory data is stored.
