#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

/* Constants */
#define VGA_WIDTH 80
#define VGA_HEIGHT 20
#define MAX_COL_WIDTH 80
#define MAX_ROW_HEIGHT 20

/* Register Adresses */
#define VIDEO_ADDRESS 0xB8000


/* Structures */

/* Extern Functions */
void kernel_main();
void print(const char* str);

#endif