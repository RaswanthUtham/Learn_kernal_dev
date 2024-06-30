#include "fat16.h"
#include "string/string.h"
#include "disk/disk.h"
#include "disk/streamer.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "status.h"
#include "kernel.h"
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

struct fat_file_descriptor /* Describes an open FAT item and its current position */
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
int fat16_read(struct disk* disk, void* descriptor, uint32_t size, uint32_t nmemb, char* out_ptr);
int fat16_seek(void *private, uint32_t offset, FILE_SEEK_MODE seek_mode);

struct filesystem fat16_fs =
{
    .resolve = fat16_resolve,
    .open = fat16_open,
    .read = fat16_read,
    .seek = fat16_seek
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
    
    /* if any errors were encountered (indicated by res being negative), free the allocated fat_private structure using kfree and set disk->fs_private to 0. */
    if (res < 0)
    {
        kfree(fat_private);
        disk->fs_private = 0;
    }
    return res;
}

void fat16_free_directory(struct fat_directory* directory)
{
    if (!directory)
    {
        return;
    }

    if (directory->item)
    {
        kfree(directory->item);
    }

    kfree(directory);
}


void fat16_fat_item_free(struct fat_item* item)
{
    if (item->type == FAT_ITEM_TYPE_DIRECTORY)
    {
        fat16_free_directory(item->directory);
    }
    else if(item->type == FAT_ITEM_TYPE_FILE)
    {
        kfree(item->item);
    }

    kfree(item);
}

/*
 * This function converts a FAT16 string (which may be padded with spaces and does not use null-termination) into a proper null-terminated C string.
 * char** out: A double pointer to the output buffer where the proper string will be written.
 * const char* in: A pointer to the input FAT16 string that may contain space-padding and no null-termination.
 */

void fat16_to_proper_string(char** out, const char* in)
{
    /* Copy Characters Until Null or Space: */
    while(*in != 0x00 && *in != 0x20)
    {
        **out = *in;
        *out += 1;
        in +=1;
    }
    
    /* Null-Terminate the Output String */
    if (*in == 0x20)
    {
        **out = 0x00;
    }
}

/*
 * This function constructs the full relative filename from a FAT16 directory item by combining its filename and extension, and writes the result to an output buffer
 * struct fat_directory_item* item: A pointer to the directory item from which to construct the filename.
 * char* out: The output buffer where the full relative filename will be stored.
 * int max_len: The maximum length of the output buffer.
 */
void fat16_get_full_relative_filename(struct fat_directory_item* item, char* out, int max_len)
{
    /* Initialize the Output Buffer: */
    memset(out, 0x00, max_len);
    char *out_tmp = out;

    /* Convert the Filename to a null terminated string */
    fat16_to_proper_string(&out_tmp, (const char*) item->filename);

    /* checks if the extension part of the directory item is not empty or filled with spaces.*/
    if (item->ext[0] != 0x00 && item->ext[0] != 0x20)
    {
        /* Append `.` for extension */
        *out_tmp++ = '.';
        /* converts the ext part of the directory item to a proper string format and appends it to the out_tmp buffer.*/
        fat16_to_proper_string(&out_tmp, (const char*) item->ext);
    }

}

static uint32_t fat16_get_first_cluster(struct fat_directory_item* item)
{
    return (item->high_16_bits_first_cluster) | item->low_16_bits_first_cluster;
}

static int fat16_cluster_to_sector(struct fat_private* private, int cluster)
{
    return private->root_directory.ending_sector_pos + ((cluster - 2) * private->header.primary_header.sectors_per_cluster);
}

static uint32_t fat16_get_first_fat_sector(struct fat_private* private)
{
    return private->header.primary_header.reserved_sectors;
}


struct fat_directory_item* fat16_clone_directory_item(struct fat_directory_item* item, int size)
{
    struct fat_directory_item* item_copy = 0;
    if (size < sizeof(struct fat_directory_item))
    {
        return 0;
    }

    item_copy = kzalloc(size);
    if (!item_copy)
    {
        return 0;
    }

    memcpy(item_copy, item, size);
    return item_copy;
}


/*
 * The function fat16_get_fat_entry is responsible for reading a specific FAT (File Allocation Table) entry for a given cluster from the disk. This function plays a crucial role in navigating the FAT file system, as it allows the system to find the next cluster in a file's cluster chain.
 * Here's a breakdown of the function, assuming we're looking for the FAT entry for cluster number 12
 * FAT table starts at sector: 1
 * Disk sector size: 4096 bytes
 * FAT entry size: 2 bytes (for FAT16)
 */

static int fat16_get_fat_entry(struct disk* disk, int cluster)
{
    int res = -1;
    struct fat_private* private = disk->fs_private;
    struct disk_stream* stream = private->fat_read_stream;
    if (!stream)
    {
        goto out;
    }
    /* Calculate FAT Table Position */
    uint32_t fat_table_position = fat16_get_first_fat_sector(private) * disk->sector_size; /* If the first FAT sector is 1, fat_table_position will be 1 * 4096 = 4096 bytes */
    /* Seek to FAT Entry */
    res = diskstreamer_seek(stream, fat_table_position * (cluster * PEACHOS_FAT16_FAT_ENTRY_SIZE)); /* For cluster 12, the position would be 4096 + (12 * 2) = 4096 + 24 = 4120 bytes */
    if (res < 0)
    {
        goto out;
    }
    
    /* Read FAT Entry */
    uint16_t result = 0;
    res = diskstreamer_read(stream, &result, sizeof(result)); /* The function reads 2 bytes from position 4120 into result */
    if (res < 0)
    {
        goto out;
    }

    res = result;
out:
    return res;
}

/*
 *  the fat16_get_cluster_for_offset function, which determines the cluster to use based on the starting cluster and a given offset within a FAT16 filesystem.
 */
static int fat16_get_cluster_for_offset(struct disk* disk, int starting_cluster, int offset)
{
    int res = 0;
    struct fat_private* private = disk->fs_private;
    int size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = starting_cluster;
    int clusters_ahead = offset / size_of_cluster_bytes;

    /* The for loop iterates clusters_ahead times to traverse the FAT table and find the cluster corresponding to the given offset. */
    for (int i = 0; i < clusters_ahead; i++)
    {
        int entry = fat16_get_fat_entry(disk, cluster_to_use);
        if (entry == 0xFF8 || entry == 0xFFF)
        {
            // We are at the last entry in the file
            res = -EIO;
            goto out;
        }

        // Sector is marked as bad?
        if (entry == PEACHOS_FAT16_BAD_SECTOR)
        {
            res = -EIO;
            goto out;
        }

        // Reserved sector?
        if (entry == 0xFF0 || entry == 0xFF6)
        {
            res = -EIO;
            goto out;
        }

        if (entry == 0x00)
        {
            res = -EIO;
            goto out;
        }
        
        /* cluster_to_use is updated to the next cluster. */
        cluster_to_use = entry;
    }

    res = cluster_to_use;
out:
    return res;
}

/* 
 * The fat16_read_internal_from_stream function is designed to read data from a specific location in a FAT16 filesystem by navigating through the clusters
 * struct disk* disk: A pointer to the disk structure.
 * struct disk_stream* stream: A stream object used to read from the disk.
 * int cluster: The starting cluster from which to read.
 * int offset: The offset within the cluster from which to start reading.
 * int total: The total number of bytes to read.
 * void* out: A pointer to the buffer where the read data will be stored.
 */
static int fat16_read_internal_from_stream(struct disk* disk, struct disk_stream* stream, int cluster, int offset, int total, void* out)
{
    int res = 0;
    struct fat_private* private = disk->fs_private;
    int size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    /* calculates which cluster to use based on the given offset. */
    int cluster_to_use = fat16_get_cluster_for_offset(disk, cluster, offset);
    if (cluster_to_use < 0)
    {
        res = cluster_to_use;
        goto out;
    }

    /*  calculates the offset within the cluster */
    int offset_from_cluster = offset % size_of_cluster_bytes;
    
    /*  converts the cluster to the starting sector. */
    int starting_sector = fat16_cluster_to_sector(private, cluster_to_use);
    /*  calculates the starting position in bytes from which to begin reading */
    int starting_pos = (starting_sector * disk->sector_size) + offset_from_cluster;
    /* determines how much data to read, limited to the size of a cluster. */
    int total_to_read = total > size_of_cluster_bytes ? size_of_cluster_bytes : total;
    /*  moves the stream to the calculated starting position. */
    res = diskstreamer_seek(stream, starting_pos);
    if (res != PEACHOS_ALL_OK)
    {
        goto out;
    }
    /*  reads the data from the disk into the output buffer. */
    res = diskstreamer_read(stream, out, total_to_read);
    if (res != PEACHOS_ALL_OK)
    {
        goto out;
    }
    /* Subtract the amount read from the total remaining to read */
    total -= total_to_read;
    if (total > 0)
    {
        // We still have more to read
        res = fat16_read_internal_from_stream(disk, stream, cluster, offset+total_to_read, total, out + total_to_read);
    }

out:
    return res;
}

static int fat16_read_internal(struct disk* disk, int starting_cluster, int offset, int total, void* out)
{
    struct fat_private* fs_private = disk->fs_private;
    struct disk_stream* stream = fs_private->cluster_read_stream;
    return fat16_read_internal_from_stream(disk, stream, starting_cluster, offset, total, out);
}

/* 
 * The fat16_load_fat_directory function loads a FAT16 directory from the disk. It takes a directory item representing the directory and reads its contents into memory.
 * struct disk* disk: A pointer to the disk structure.
 * struct fat_directory_item* item: A pointer to the FAT16 directory item structure that represents the directory to be loaded.
 */
struct fat_directory* fat16_load_fat_directory(struct disk* disk, struct fat_directory_item* item)
{
    int res = 0;
    struct fat_directory* directory = 0;
    struct fat_private* fat_private = disk->fs_private;

    /* Check if the Item is a Subdirectory */
    if (!(item->attribute & FAT_FILE_SUBDIRECTORY))
    {
        res = -EINVARG;
        goto out;
    }
    
    /* Allocate Memory for the Directory */
    directory = kzalloc(sizeof(struct fat_directory));
    if (!directory)
    {
        res = -ENOMEM;
        goto out;
    }
    
    /* Get the first cluster of the directory using fat16_get_first_cluster. */
    int cluster = fat16_get_first_cluster(item);
    /* Convert the cluster to a sector number using fat16_cluster_to_sector. */
    int cluster_sector = fat16_cluster_to_sector(fat_private, cluster);
    /* Get the total number of items in the directory */
    int total_items = fat16_get_total_items_for_directory(disk, cluster_sector);
    directory->total = total_items;
    /* Allocate memory for the directory items */
    int directory_size = directory->total * sizeof(struct fat_directory_item);
    directory->item = kzalloc(directory_size);
    if (!directory->item)
    {
        res = -ENOMEM;
        goto out;
    }
    
    /* Read the directory items from the disk */
    res = fat16_read_internal(disk, cluster, 0x00, directory_size, directory->item);
    if (res != PEACHOS_ALL_OK)
    {
        goto out;
    }


out:
    if (res != PEACHOS_ALL_OK)
    {
        fat16_free_directory(directory);
    }
    return directory;
}

/* 
 * This function creates a new fat_item structure based on a provided FAT16 directory entry (fat_directory_item). It determines whether the item is a file or a directory and initializes the fat_item structure accordingly.
 * struct disk* disk: A pointer to the disk structure.
 * struct fat_directory_item* item: A pointer to the FAT16 directory item structure that represents the file or directory.
 */
struct fat_item* fat16_new_fat_item_for_directory_item(struct disk* disk, struct fat_directory_item* item)
{
    /* Allocate Memory for fat_item */
    struct fat_item* f_item = kzalloc(sizeof(struct fat_item));
    if (!f_item)
    {
        return 0;
    }
    
    /* Check if the Item is a Subdirectory */
    if (item->attribute & FAT_FILE_SUBDIRECTORY)
    {
        f_item->directory = fat16_load_fat_directory(disk, item);
        f_item->type = FAT_ITEM_TYPE_DIRECTORY;
    }
    /* Otherwise, Set as a File */
    else
    {
        f_item->type = FAT_ITEM_TYPE_FILE;
        f_item->item = fat16_clone_directory_item(item, sizeof(struct fat_directory_item));
    }
    return f_item;
}


/* 
 * The function searches for an item (file or directory) with a specific name within a given directory on a FAT16 filesystem 
 * struct disk* disk: A pointer to the disk structure representing the physical disk.
 * struct fat_directory* directory: A pointer to the directory structure where the search will be performed.
 * const char* name: The name of the item to search for within the directory.
 */
struct fat_item* fat16_find_item_in_directory(struct disk* disk, struct fat_directory* directory, const char* name)
{
    struct fat_item* f_item = 0;
    char tmp_filename[PEACHOS_MAX_PATH];

    /* Loop Through Directory Items: */
    for (int i = 0; i < directory->total; i++)
    {
        fat16_get_full_relative_filename(&directory->item[i], tmp_filename, sizeof(tmp_filename));
        if (istrncmp(tmp_filename, name, sizeof(tmp_filename)) == 0)
        {
            // Found it let's create a new fat_item
            f_item = fat16_new_fat_item_for_directory_item(disk, &directory->item[i]);
        }
    }

    return f_item;
}

/*
 * The fat16_get_directory_entry function is designed to locate and return a FAT16 directory entry for a specified file path. It navigates through the directory structure of the FAT16 filesystem to find the requested file or directory
 * struct disk* disk: A pointer to the disk structure containing the FAT16 filesystem.
 * struct path_part* path: A pointer to the path structure representing the file path to locate
 */
struct fat_item* fat16_get_directory_entry(struct disk* disk, struct path_part* path)
{
    /* Retrieves the FAT16-specific data from the disk structure. */
    struct fat_private* fat_private = disk->fs_private;

    /*  Initializes a pointer to fat_item to NULL. */
    struct fat_item* current_item = 0;

    /* find the initial part of the path within the root directory. */
    struct fat_item* root_item = fat16_find_item_in_directory(disk, &fat_private->root_directory, path->part);
    if (!root_item)
    {
        goto out;
    }
    
    /* Points to the next part of the path. */
    struct path_part* next_part = path->next;

    /* Initializes current_item to root_item. */
    current_item = root_item;

    /* Loops through each part of the path until there are no more parts. */
    while(next_part != 0)
    {
        /* Checks if the current item is not a directory. If it's not, the path is invalid, and current_item is set to NULL. */
        if (current_item->type != FAT_ITEM_TYPE_DIRECTORY)
        {
            current_item = 0;
            break;
        }
        
        /* Searches for the next part of the path within the current directory. */
        struct fat_item* tmp_item = fat16_find_item_in_directory(disk, current_item->directory, next_part->part);
        /* Frees the current item to avoid memory leaks. */
        fat16_fat_item_free(current_item);
        /* Updates current_item to the found item */
        current_item = tmp_item;
        /* Moves to the next part of the path. */
        next_part = next_part->next;
    }
out:
    return current_item;
}

/*
 * This function is responsible for opening a file in the FAT16 filesystem. It validates the file mode, allocates a file descriptor, and initializes it with the file's directory entry.
 * struct disk* disk: A pointer to the disk structure on which the FAT16 filesystem resides.
 * struct path_part* path: A pointer to the path structure representing the file to be opened.
 * FILE_MODE mode: An enumeration value representing the mode in which to open the file (e.g., read, write).
 */
void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode)
{
    /* Check File Mode */
    if (mode != FILE_MODE_READ)
    {
        return ERROR(-ERDONLY);
    }

    /* Allocate File Descriptor */
    struct fat_file_descriptor* descriptor = 0;
    descriptor = kzalloc(sizeof(struct fat_file_descriptor));
    if (!descriptor)
    {
        return ERROR(-ENOMEM);
    }
    
    /* Get Directory Entry */
    descriptor->item = fat16_get_directory_entry(disk, path);
    if (!descriptor->item)
    {
        return ERROR(-EIO);
    }

    descriptor->pos = 0;
    return descriptor;
}

/*
 * The fat16_read function is designed to read data from a FAT16 filesystem
 */
int fat16_read(struct disk* disk, void* descriptor, uint32_t size, uint32_t nmemb, char* out_ptr)
{
    int res = 0;
    struct fat_file_descriptor* fat_desc = descriptor;
    struct fat_directory_item* item = fat_desc->item->item;
    int offset = fat_desc->pos; // Initial offset from the descriptor's position
    for (uint32_t i = 0; i < nmemb; i++)
    {
        res = fat16_read_internal(disk, fat16_get_first_cluster(item), offset, size, out_ptr);
        if (ISERR(res))
        {
            goto out;
        }

        out_ptr += size;
        offset += size; // Update the offset after each read
    }
    fat_desc->pos = offset;  // Update the descriptor's position after reading
    res = nmemb;
out:
    return res;
}

int fat16_seek(void *private, uint32_t offset, FILE_SEEK_MODE seek_mode)
{
    int res = 0;
    struct fat_file_descriptor *desc = private;
    struct fat_item *desc_item = desc->item;
    if (desc_item->type != FAT_ITEM_TYPE_FILE)
    {
        res = -EINVARG;
        goto out;
    }

    struct fat_directory_item *ritem = desc_item->item;
    if (offset >= ritem->filesize)
    {
        res = -EIO;
        goto out;
    }

    switch (seek_mode)
    {
    case SEEK_SET:
        desc->pos = offset;
        break;

    case SEEK_END:
        res = -EUNIMP;
        break;

    case SEEK_CUR:
        desc->pos += offset;
        break;

    default:
        res = -EINVARG;
        break;
    }
out:
    return res;
}
