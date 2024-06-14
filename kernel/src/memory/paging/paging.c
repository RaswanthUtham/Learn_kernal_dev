#include "paging.h"
#include "status.h"
#include "memory/heap/kheap.h"

void paging_load_directory(uint32_t* directory); /* asm function */

static uint32_t* current_directory = 0;

struct paging_4gb_chunk* paging_new_4gb(uint8_t flags)
{
    uint32_t* directory = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE); /* page dir */
    int offset = 0;
    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++)
    {
        uint32_t* entry = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE); /* page table */
        for (int b = 0; b < PAGING_TOTAL_ENTRIES_PER_TABLE; b++)
        {
            entry[b] = (offset + (b * PAGING_PAGE_SIZE)) | flags; /* page size */
        }
        offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE); /* page table size = page size * page table entries */
        directory[i] = (uint32_t)entry | flags | PAGING_IS_WRITEABLE;
    }

    struct paging_4gb_chunk* chunk_4gb = kzalloc(sizeof(struct paging_4gb_chunk));
    chunk_4gb->directory_entry = directory;
    return chunk_4gb;
}

void paging_switch(uint32_t* directory)
{
    paging_load_directory(directory);
    current_directory = directory;
}

uint32_t* paging_4gb_chunk_get_directory(struct paging_4gb_chunk* chunk)
{
    return chunk->directory_entry;
}

bool paging_is_aligned(void* addr)
{
    return ((uint32_t)addr % PAGING_PAGE_SIZE) == 0;
}

int paging_get_indexes(void* virtual_address, uint32_t* directory_index_out, uint32_t* table_index_out)
{
    int res = 0;
    if (!paging_is_aligned(virtual_address))
    {
        res = -EINVARG;
        goto out;
    }
    
    /* directory index = virtual address / page table size */
    *directory_index_out = ((uint32_t)virtual_address / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE));

    /* table index = virtual address % page table size / page size */
    *table_index_out = ((uint32_t) virtual_address % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE) / PAGING_PAGE_SIZE);
out:
    return res;
}

int paging_set(uint32_t* directory, void* virt, uint32_t val)
{
    /* map absolute address (val) to virtual address (virt) */

    if (!paging_is_aligned(virt))
    {
        return -EINVARG;
    }

    uint32_t directory_index = 0;
    uint32_t table_index = 0;
    int res = paging_get_indexes(virt, &directory_index, &table_index);
    if (res < 0)
    {
        return res;
    }
    
    /* Retrieves the directory entry from the page directory using the directory_index */
    uint32_t entry = directory[directory_index];

    /* Calculates the base address of the page table by masking the lower 12 bits of the entry to zero (ensuring it is aligned to a page boundary) */
    uint32_t* table = (uint32_t*)(entry & 0xfffff000);

    /* Map the absolute address (val) to the page */
    table[table_index] = val;

    return 0;
}
