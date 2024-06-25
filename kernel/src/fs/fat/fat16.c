#include "fat16.h"
#include "string/string.h"
#include "disk/disk.h"
#include "disk/streamer.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "status.h"
#include <stdint.h>

#define PEACHOS_FAT16_SIGNATURE 0x29
#define PEACHOS_FAT16_FAT_ENTRY_SIZE 0x02
#define PEACHOS_FAT16_BAD_SECTOR 0xFF7
#define PEACHOS_FAT16_UNUSED 0x00


typedef unsigned int FAT_ITEM_TYPE;
#define FAT_ITEM_TYPE_DIRECTORY 0
#define FAT_ITEM_TYPE_FILE 1

// Fat directory entry attributes bitmask
#define FAT_FILE_READ_ONLY 0x01
#define FAT_FILE_HIDDEN 0x02
#define FAT_FILE_SYSTEM 0x04
#define FAT_FILE_VOLUME_LABEL 0x08
#define FAT_FILE_SUBDIRECTORY 0x10
#define FAT_FILE_ARCHIVED 0x20
#define FAT_FILE_DEVICE 0x40
#define FAT_FILE_RESERVED 0x80


struct fat_header_extended
{
    uint8_t drive_number;
    uint8_t win_nt_bit;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_id_string[11];
    uint8_t system_id_string[8];
} __attribute__((packed));

struct fat_header
{
    uint8_t short_jmp_ins[3];
    uint8_t oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_copies;
    uint16_t root_dir_entries;
    uint16_t number_of_sectors;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_setors;
    uint32_t sectors_big;
} __attribute__((packed));

struct fat_h /* Combines the primary and extended headers into a single structure. */
{
    struct fat_header primary_header;
    union fat_h_e
    {
        struct fat_header_extended extended_header;
    } shared;
};

struct fat_directory_item /* Represents a directory entry in a FAT file system. */
{
    uint8_t filename[8];
    uint8_t ext[3];
    uint8_t attribute;
    uint8_t reserved;
    uint8_t creation_time_tenths_of_a_sec;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access;
    uint16_t high_16_bits_first_cluster;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t low_16_bits_first_cluster;
    uint32_t filesize;
} __attribute__((packed));

struct fat_directory /* Represents a directory containing multiple directory items. */
{
    struct fat_directory_item* item;
    int total;
    int sector_pos;
    int ending_sector_pos;
};

struct fat_item /* Represents a FAT item, which can be either a file or a directory. */
{
    union 
    {
        struct fat_directory_item* item;
        struct fat_directory* directory;
    };
    
    FAT_ITEM_TYPE type;
};

struct fat_item_descriptor /* Describes an open FAT item and its current position */
{
    struct fat_item* item;
    uint32_t pos;
};

/* Holds private data for the FAT file system, including the header, root directory, and streams for reading clusters, the FAT, and directories. */
struct fat_private
{
    struct fat_h header;
    struct fat_directory root_directory;

    // Used to stream data clusters
    struct disk_stream* cluster_read_stream;
    // Used to stream the file allocation table
    struct disk_stream* fat_read_stream;


    // Used in situations where we stream the directory
    struct disk_stream* directory_stream;
};

int fat16_resolve(struct disk* disk);
void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode);

struct filesystem fat16_fs =
{
    .resolve = fat16_resolve,
    .open = fat16_open
};

struct filesystem* fat16_init()
{
    strcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
}

static void fat16_init_private(struct disk* disk, struct fat_private* private)
{
    memset(private, 0, sizeof(struct fat_private));
    private->cluster_read_stream = diskstreamer_new(disk->id);
    private->fat_read_stream = diskstreamer_new(disk->id);
    private->directory_stream = diskstreamer_new(disk->id);
}


int fat16_sector_to_absolute(struct disk* disk, int sector)
{
    return sector * disk->sector_size;
}

/* 
 * This function counts the total number of valid items in a FAT16 directory starting at a specified sector on the disk.
 * struct disk* disk: A pointer to a disk structure representing the disk.
 * uint32_t directory_start_sector: The starting sector of the directory.
 */

int fat16_get_total_items_for_directory(struct disk* disk, uint32_t directory_start_sector)
{
    struct fat_directory_item item; /* A variable to hold directory items read from the disk */
    struct fat_directory_item empty_item; /*  A variable to represent an empty directory item. It is initialized to zero using memset. */
    memset(&empty_item, 0, sizeof(empty_item));
    
    struct fat_private* fat_private = disk->fs_private; /*  Retrieves the FAT16 private data from the disk structure. */

    int res = 0;
    int i = 0;
    int directory_start_pos = directory_start_sector * disk->sector_size; /* : Calculates the starting position of the directory in bytes */
    struct disk_stream* stream = fat_private->directory_stream;
    /* Seek to the Directory Start Position */
    if(diskstreamer_seek(stream, directory_start_pos) != PEACHOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    /* Read and Count Directory Items */
    while(1)
    {
        /* Reads a directory item from the disk into the item */
        if (diskstreamer_read(stream, &item, sizeof(item)) != PEACHOS_ALL_OK)
        {
            res = -EIO;
            goto out;
        }

        /*  Checks if the filename of the directory item is 0x00, which indicates the end of the directory. If so, break out of the loop */
        if (item.filename[0] == 0x00)
        {
            // We are done
            break;
        }

        /* Checks if the filename of the directory item is 0xE5, which indicates that the item is unused. If so, skip this item and continue to the next iteration. */
        if (item.filename[0] == 0xE5)
        {
            continue;
        }

        i++;
    }
    
    /* Sets the result to the number of valid items counted. */
    res = i;

out:
    return res;
}

/*
 *  This function loads the root directory of a FAT16 filesystem from a given disk into a fat_directory structure.
 *  struct disk* disk: A pointer to a disk structure representing the disk.
 *  struct fat_private* fat_private: A pointer to a fat_private structure containing private data for the FAT16 filesystem.
 *  struct fat_directory* directory: A pointer to a fat_directory structure where the root directory will be loaded.
 */

int fat16_get_root_directory(struct disk* disk, struct fat_private* fat_private, struct fat_directory* directory)
{
    int res = 0;
    struct fat_header* primary_header = &fat_private->header.primary_header;

    /* Calculates the starting sector position of the root directory. It is located after the FAT copies and reserved sectors. */
    int root_dir_sector_pos = (primary_header->fat_copies * primary_header->sectors_per_fat) + primary_header->reserved_sectors;

    /* Retrieves the number of root directory entries from the primary header. */
    int root_dir_entries = fat_private->header.primary_header.root_dir_entries;

    /*  Calculates the total size of the root directory in bytes. */
    int root_dir_size = (root_dir_entries * sizeof(struct fat_directory_item));

    /* Calculates the total number of sectors needed to store the root directory. */
    int total_sectors = root_dir_size / disk->sector_size;

    /* If the root directory size is not a multiple of the sector size, add an additional sector to accommodate the remaining bytes. */
    if (root_dir_size % disk->sector_size)
    {
        total_sectors += 1;
    }
    
    /* Get Total Items in the Root Directory */
    int total_items = fat16_get_total_items_for_directory(disk, root_dir_sector_pos);

    /* Allocate Memory for Root Directory Items */
    struct fat_directory_item* dir = kzalloc(root_dir_size);
    if (!dir)
    {
        res = -ENOMEM;
        goto out;
    }
    
    /* Seek to Root Directory Position */
    struct disk_stream* stream = fat_private->directory_stream;
    if (diskstreamer_seek(stream, fat16_sector_to_absolute(disk, root_dir_sector_pos)) != PEACHOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    /* Reads the root directory from the disk into the allocated memory (dir) */
    if (diskstreamer_read(stream, dir, root_dir_size) != PEACHOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }
    
    /* Sets the item field of the directory structure to point to the allocated memory containing the root directory items. */
    directory->item = dir;
    /* Sets the total field to the total number of items in the root directory. */
    directory->total = total_items;
    /*  Sets the sector_pos field to the starting sector position of the root directory. */
    directory->sector_pos = root_dir_sector_pos;
    /* Sets the ending_sector_pos field to the ending sector position of the root directory. */
    directory->ending_sector_pos = root_dir_sector_pos + (root_dir_size / disk->sector_size);
out:
    return res;
}

/*
 * The purpose of this function is to initialize and verify that a given disk contains a valid FAT16 filesystem. It also prepares necessary structures for further interactions with the filesystem.
 * struct disk* disk: A pointer to a disk structure that represents the disk to be resolved.
 */
int fat16_resolve(struct disk* disk)
{
    int res = 0;
    struct fat_private* fat_private = kzalloc(sizeof(struct fat_private));

    /* Initializes the fat_private structure with necessary data streams and zeroes out the rest. */
    fat16_init_private(disk, fat_private);
    
    /* Sets the fs_private field of the disk to point to the newly allocated and initialized fat_private structure. */
    disk->fs_private = fat_private;

    /* Sets the filesystem field of the disk to point to the fat16_fs structure, which contains pointers to FAT16-specific functions. */
    disk->filesystem = &fat16_fs;
    
    /*  Creates a new stream for reading from the disk. */
    struct disk_stream* stream = diskstreamer_new(disk->id);
    if(!stream)
    {
        res = -ENOMEM;
        goto out;
    }
    
    /* Reads the FAT header from the disk into fat_private->header */
    if (diskstreamer_read(stream, &fat_private->header, sizeof(fat_private->header)) != PEACHOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }
    
    /* Verify FAT16 Signature */
    if (fat_private->header.shared.extended_header.signature != 0x29)
    {
        res = -EFSNOTUS;
        goto out;
    }

    /* Loads the root directory of the FAT16 filesystem into fat_private->root_directory */
    if (fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) != PEACHOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

out:
    /*  If the stream was successfully created, close it using diskstreamer_close. */
    if (stream)
    {
        diskstreamer_close(stream);
    }
    
    /* f any errors were encountered (indicated by res being negative), free the allocated fat_private structure using kfree and set disk->fs_private to 0. */
    if (res < 0)
    {
        kfree(fat_private);
        disk->fs_private = 0;
    }
    return res;
}

void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode)
{
    return 0;
}
