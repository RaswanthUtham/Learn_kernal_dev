#include "kernel.h"
#include <stddef.h>
#include <stdint.h>
#include "idt/idt.h"
#include "io/io.h"
#include "string/string.h"
#include "disk/disk.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "fs/pparser.h"

uint16_t* video_mem = 0;
uint16_t terminal_row = 0;
uint16_t terminal_col = 0;

void error_1();
void error_2();

uint16_t terminal_make_char(char c, char colour)
{
    return (colour << 8) | c; /* 0th byte c, 1st byte color */
}

void terminal_putchar(int x, int y, char c, char colour)
{
    video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(c, colour); /* (row * max_column + column) */
}

void terminal_writechar(char c, char colour)
{
    if (c == '\n')
    {
        terminal_row += 1;
        terminal_col = 0;
        return;
    }
    
    terminal_putchar(terminal_col, terminal_row, c, colour);
    terminal_col += 1;
    if (terminal_col >= VGA_WIDTH)
    {
        terminal_col = 0;
        terminal_row += 1;
    }
}

void terminal_initialize()
{
    video_mem = (uint16_t*)(0xB8000);
    terminal_row = 0;
    terminal_col = 0;
    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            terminal_putchar(x, y, ' ', 0);
        }
    }   
}


void print(const char* str)
{
    size_t len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        terminal_writechar(str[i], 15);
    }
}

static struct paging_4gb_chunk* kernel_chunk = 0; /* kernel page directory */

void kernel_main()
{
    terminal_initialize();
    print("Hello world!\ntest");

    /* heap init */
    kheap_init();

    /* search and initialise hdd and disks */
    disk_search_and_init();

    /* create page directory for kernel */
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
   
    /* switch to kernel page directory */
    paging_switch(paging_4gb_chunk_get_directory(kernel_chunk));
    
    /* get ptr from heap. It is a physical address 0x1000000 */
    char* ptr = kzalloc(4096);

    /* map the physical addr 0x1000000 to 0x1000 */
    paging_set(paging_4gb_chunk_get_directory(kernel_chunk), (void*)0x1000, (uint32_t)ptr | PAGING_ACCESS_FROM_ALL | PAGING_IS_PRESENT | PAGING_IS_WRITEABLE);

    /* Enable Paging */
    enable_paging();

    /* IDT Init */
    idt_init();

    /* enable interrupt */
    enable_interrupts();

    /* IDT test */
//    error_1();
//    error_2();

    /* IO test */
//    outb(0x60/*port*/, 0xff/*value*/);

    /* heap- test */
     // void *ptr1 = kmalloc(20);
     // void *ptr2 = kmalloc(5000);
     // void *ptr3 = kmalloc(12000);
     // void *ptr4 = kmalloc(2);
     // if (ptr1 || ptr2 || ptr3 || ptr4)
     // {
     //     // ptr1 = (void *) 0x1000000
     //     // ptr2 = (void *) 0x1001000
     //     // ptr3 = (void *) 0x1003000
     //     // ptr4 = (void *) 0x1006000
     // }

    /* Malloc Test 2*/
     // void *ptr1 = kmalloc(5000);  // 000-1999 2
     // void *ptr2 = kmalloc(50);    //  2000 - 2999 1
     // kfree(ptr2);                 // 2000 - 2999 1
     // kfree(ptr1);                 // 000 - 1999 2
     // void *ptr3 = kmalloc(12000); // 000 - 2999 3
     // void *ptr4 = kmalloc(2);     // 3000 - 3999
     // void *ptr5 = kmalloc(5);     // 4000 - 4999
     // if (ptr1 || ptr2 || ptr3 || ptr4 || ptr5)
     // {
     // }

    /* Malloc Test 3*/
    //  void *ptr1 = kmalloc(5000);  // 000-1999 2
    //  void *ptr2 = kmalloc(50);    //  2000 - 2999 1
    //  kfree(ptr2);                 // 2000 - 2999 1
    //  kfree(ptr1);                 // 000 - 1999 2
    //  void *ptr3 = kmalloc(12000); // 000 - 2999 3
    //  void *ptr4 = kmalloc(2);     // 3000 - 3999
    //  void *ptr5 = kmalloc(5);     // 4000 - 4999
    //  kfree(ptr3);                 // 0 - 2999
    //  void *ptr6 = kmalloc(3900);  // 000 - 999
    //  void *ptr7 = kmalloc(8000);  // 1000 - 2999
    //  void *ptr8 = kmalloc(199);   // 5000 - 5999
    //  if (ptr1 || ptr2 || ptr3 || ptr4 || ptr5 || ptr6 || ptr7 || ptr8)
    //  {
    //  }
    
    /* paging test */
   //  char *ptr2 = (char *) 0x1000;
   //  ptr2[0] = 'Z';
   //  ptr2[1] = 'p';

   //  print(ptr2);
   //  print(ptr);
   
    /* pathparser test */
    struct path_root* root_path = pathparser_parse("0:/bin/shell.exe", NULL);

    if(root_path)
    {


    }
}
