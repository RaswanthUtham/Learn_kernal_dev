#include "file.h"
#include "config.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "status.h"
#include "kernel.h"

/* global variables */
struct filesystem* filesystems[PEACHOS_MAX_FILESYSTEMS]; /* filesystems is an array of pointers to filesystem structures, used to keep track of the various file systems registered in the OS. */
struct file_descriptor* file_descriptors[PEACHOS_MAX_FILE_DESCRIPTORS]; /* file_descriptors is an array of pointers to file_descriptor structures, used to manage file descriptors for open files. */ 


/* This function finds the first available slot in the filesystems array and returns a pointer to that slot. If no free slot is found, it returns 0. */

static struct filesystem** fs_get_free_filesystem()
{
    int i = 0;
    for (i = 0; i < PEACHOS_MAX_FILESYSTEMS; i++)
    {
        if (filesystems[i] == 0)
        {
            return &filesystems[i];
        }
    }

    return 0;
}

/* This function inserts a new file system into the filesystems array. It first retrieves a free slot using fs_get_free_filesystem. If no free slot is found, it prints an error message and enters an infinite loop. Otherwise, it assigns the new filesystem to the free slot. */

void fs_insert_filesystem(struct filesystem* filesystem)
{
    struct filesystem** fs;
    fs = fs_get_free_filesystem();
    if (!fs)
    {
        print("Problem inserting filesystem");
        while(1) {}
    }

    *fs = filesystem;
}

static void fs_static_load()
{
    // fs_insert_filesystem(fat16_init());
}

/* This function initializes the filesystems array to zero, clearing any previously registered file systems, and then calls fs_static_load to load any static file systems. */

void fs_load()
{
    memset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}

/* This function initializes the file_descriptors array to zero, clearing any open file descriptors, and then calls fs_load to initialize the file systems */

void fs_init()
{
    memset(file_descriptors, 0, sizeof(file_descriptors));
    fs_load();
}

/* This function allocates a new file descriptor. It searches for a free slot in the file_descriptors array, allocates memory for a new descriptor using kzalloc, and assigns the descriptor an index. If successful, it returns 0 and sets desc_out to point to the new descriptor. Otherwise, it returns an error code (-ENOMEM). */

static int file_new_descriptor(struct file_descriptor** desc_out)
{
    int res = -ENOMEM;
    for (int i = 0; i < PEACHOS_MAX_FILE_DESCRIPTORS; i++)
    {
        if (file_descriptors[i] == 0)
        {
            struct file_descriptor* desc = kzalloc(sizeof(struct file_descriptor));
            // Descriptors start at 1
            desc->index = i + 1;
            file_descriptors[i] = desc;
            *desc_out = desc;
            res = 0;
            break;
        }
    }

    return res;
}

/* This function retrieves a file descriptor based on its file descriptor number (fd). It checks if the fd is within valid range and returns the corresponding file descriptor */

static struct file_descriptor* file_get_descriptor(int fd)
{
    if (fd <= 0 || fd >= PEACHOS_MAX_FILE_DESCRIPTORS)
    {
        return 0;
    }

    // Descriptors start at 1
    int index = fd - 1;
    return file_descriptors[index];
}

/* This function tries to resolve which file system is present on a given disk. It iterates through the filesystems array and calls the resolve function of each file system. If a file system can handle the disk, it returns a pointer to that file system. */
struct filesystem* fs_resolve(struct disk* disk)
{
    struct filesystem* fs = 0;
    for (int i = 0; i < PEACHOS_MAX_FILESYSTEMS; i++)
    {
        if (filesystems[i] != 0 && filesystems[i]->resolve(disk) == 0)
        {
            fs = filesystems[i];
            break;
        }
    }

    return fs;
}

int fopen(const char* filename, const char* mode)
{
    return -EIO;
}
