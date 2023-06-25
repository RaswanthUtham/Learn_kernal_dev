all:
	nasm -f bin ./boot/boot.asm -o ./boot/boot.bin
	dd if=./read_disk.txt >> ./boot/boot.bin
	dd if=/dev/zero bs=512 count=1 >> ./boot/boot.bin