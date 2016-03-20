#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "filesys/cache.h"
#include "filesys/directory.h"
#include <stdio.h>

/*! Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

/* Constants for number of entries */
#define NUM_DIRECT 122
#define NUM_BYTES_PER_SECTOR 512
#define NUM_ENTRIES NUM_BYTES_PER_SECTOR / 4
#define METADATA_BYTES (NUM_ENTRIES - NUM_DIRECT) * 4

/*! On-disk inode.
    Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk {
    block_sector_t start;               /*!< First data sector. */
    off_t length;                       /*!< File size in bytes. */
    unsigned magic;                     /*!< Magic number. */
    bool isDirectory;                   /*!< Specifies if inode is directory. */
    block_sector_t direct[NUM_DIRECT];  /*!< Array of direct blocks. */
    block_sector_t indirect;            /*!< Indirect block. */
    block_sector_t double_indirect;     /*!< Double indirect block. */
};

/*! On-disk inode for indirect and double indirect blocks.
    Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct indirect_inode_disk {
    block_sector_t sectors[NUM_ENTRIES];    /*!< Block sectors. If inode is
                                              indirect block, contains direct
                                              blocks. Else if inode is double
                                              indirect block, contains indirect
                                              blocks. */
};

/*! Returns the number of sectors to allocate for an inode SIZE
    bytes long. */
static inline size_t bytes_to_sectors(off_t size) {
    return DIV_ROUND_UP(size, BLOCK_SECTOR_SIZE);
}

/*! In-memory inode. */
struct inode {
    struct list_elem elem;              /*!< Element in inode list. */
    block_sector_t sector;              /*!< Sector number of disk location. */
    int open_cnt;                       /*!< Number of openers. */
    bool removed;                       /*!< True if deleted, false otherwise. */
    int deny_write_cnt;                 /*!< 0: writes ok, >0: deny writes. */
    struct lock extension_lock;         /*!< Lock to allow extending file. */
};

/*! Returns the minimum of a and b. If a == b, returns b. */
static inline int min(int a, int b)
{
    return a < b ? a : b;
}

/*! Returns the block device sector that contains byte offset POS
    within INODE.
    Returns -1 if INODE does not contain data for a byte at offset
    POS. */
static block_sector_t byte_to_sector(const struct inode *inode, off_t pos) {
    ASSERT(inode != NULL);

    /* Value to return */
    block_sector_t result;

    /* Read the sector for the inode containing the file metadata */
    cache_sector * sector = cache_read_sector(inode->sector);
    struct inode_disk * data = (struct inode_disk *) sector->data;

    /* The allocated length of the file is the number of sectors allocated times
     * the number of bytes per sector */
    int length = DIV_ROUND_UP(data->length, NUM_BYTES_PER_SECTOR) * NUM_BYTES_PER_SECTOR;

    /* Check if pos is valid */
    if (pos < length)
    {
        /* Convert the byte offset to a sector offset */
        int block_index = pos / NUM_BYTES_PER_SECTOR;

        /* If the desired sector offset is less than NUM_DIRECT, look in the
         * direct blocks of the inode */
        if (block_index < NUM_DIRECT)
        {
            result = data->direct[block_index];
            done_read(&sector->rw);
            return result;
        }

        /* The desired sector offset is past the number of direct blocks */
        else
        {
            /* Get the index of the desired sector past the first NUM_DIRECT
             * blocks */
            block_index -= NUM_DIRECT;

            /* If the desired sector index is less than NUM_ENTRIES, look in
             * the indirect block of the inode */
            if (block_index < NUM_ENTRIES)
            {
                /* Read the indirect block */
                cache_sector * indirect_sector = cache_read_sector(data->indirect);
                struct indirect_inode_disk * indirect_data = (struct indirect_inode_disk *) indirect_sector->data;

                /* Read the block sector number at the sector index */
                result = indirect_data->sectors[block_index];

                /* We are done reading the indirect sector and the file inode */
                done_read(&indirect_sector->rw);
                done_read(&sector->rw);
                return result;
            }

            /* The desired sector offset is past the indirect block */
            else
            {
                /* Get the index of the desired sector past the indirect block */
                block_index -= NUM_ENTRIES;

                /* If the desired sector index is less than the number of direct
                 * blocks in the double indirect block, look in the double
                 * indirect block of the inode */
                if (block_index < NUM_ENTRIES * NUM_ENTRIES)
                {
                    /* Find the index of the indirect block in the double
                     * indirect block */
                    int indirect_index = block_index / NUM_ENTRIES;

                    /* Read the double indirect block */
                    cache_sector * d_indirect_sector = cache_read_sector(data->double_indirect);
                    struct indirect_inode_disk * double_indirect_data = (struct indirect_inode_disk *) d_indirect_sector->data;

                    /* Get the block number of the indirect block */
                    block_sector_t indirect_sector_id = double_indirect_data->sectors[indirect_index];

                    /* Find the direct block indirect in the indirect block */
                    int direct_index = block_index % NUM_ENTRIES;

                    /* Read the indirect block */
                    cache_sector * indirect_sector = cache_read_sector(indirect_sector_id);
                    struct indirect_inode_disk * indirect_data = (struct indirect_inode_disk *) indirect_sector->data;

                    /* Read the block sector number at the direct index */
                    result = indirect_data->sectors[direct_index];

                    /* We are done reading the double indirect sector, the
                     * indirect sector, and the file inode */
                    done_read(&d_indirect_sector->rw);
                    done_read(&indirect_sector->rw);
                    done_read(&sector->rw);
                    return result;
                }

                /* The offset is past the double indirect block. This will not
                 * happen since we check if the offset is valid */
                else
                {
                    done_read(&sector->rw);
                    return -1;
                }
            }
        }
    }

    /* The offset is invalid */
    else {
        done_read(&sector->rw);
        return -1;
    }

}

/*! List of open inodes, so that opening a single inode twice
    returns the same `struct inode'. */
static struct list open_inodes;

/*! Initializes the inode module. */
void inode_init(void) {
    list_init(&open_inodes);
}

/*! Initializes an inode with LENGTH bytes of data and
    writes the new inode to sector SECTOR on the file system
    device.
    Returns true if successful.
    Returns false if memory or disk allocation fails. */
bool inode_create(block_sector_t sector, off_t length) {
    struct inode_disk *disk_inode = NULL;
    bool success = false;

    ASSERT(length >= 0);

    /* If this assertion fails, the inode structure is not exactly
       one sector in size, and you should fix that. */
    ASSERT(sizeof *disk_inode == BLOCK_SECTOR_SIZE);

    disk_inode = calloc(1, sizeof *disk_inode);
    if (disk_inode != NULL) {
        
        static char zeros[BLOCK_SECTOR_SIZE];

        /* Find the number of sectors needed to allocate */
        size_t sectors = bytes_to_sectors(length);

        /* Update the file metadata */
        disk_inode->length = length;
        disk_inode->magic = INODE_MAGIC;
        disk_inode->isDirectory = false;

        /* The number of direct blocks needed to allocate is the minimum of
         * the number of sectors needed and the number of direct blocks
         * available in the inode */
        int num_direct_blocks = min(sectors, NUM_DIRECT);

        /* Allocate direct blocks */
        int i;
        for (i = 0; i < num_direct_blocks; i++)
        {
            /* Allocate a free block by finding the first free block. If this 
             * fails, there is no more space. Write the inode to disk and
             * return false */
            if (!free_map_allocate(1, &disk_inode->direct[i]))
            {
                block_write(fs_device, sector, disk_inode);
                free(disk_inode);
                return success;
            }

            /* Write zeros to the new allocated block */
            block_write(fs_device, disk_inode->direct[i], zeros);
        }

        /* Find the number of remaining sectors to allocate */
        sectors -= num_direct_blocks;

        /* If we still have sectors left to allocate, begin allocating in the
         * indirect block */
        if (sectors > 0)
        {
            /* Allocate indirect block */
            struct indirect_inode_disk * ind_disk_inode = NULL;
            ind_disk_inode = calloc(1, sizeof * ind_disk_inode);

            /* Allocate a free block by finding the first free block. If this 
             * fails, there is no more space. Write the inode to disk and
             * return false */
            if (!free_map_allocate(1, &disk_inode->indirect))
            {
                free(ind_disk_inode);
                block_write(fs_device, sector, disk_inode);
                free(disk_inode);
                return success;
            }

            /* The number of direct blocks needed to allocate is the minimum of
             * the number of sectors needed and the number of direct blocks
             * available in the indirect block */
            int num_blocks = min(sectors, NUM_ENTRIES);
            for (i = 0; i < num_blocks; i++)
            {
                /* Allocate a free block by finding the first free block. If this 
                 * fails, there is no more space. Write the indirect block and
                 * inode to disk and return false */
                if (!free_map_allocate(1, &ind_disk_inode->sectors[i]))
                {
                    block_write(fs_device, disk_inode->indirect, ind_disk_inode);
                    free(ind_disk_inode);
                    block_write(fs_device, sector, disk_inode);
                    free(disk_inode);
                    return success;
                }

                /* Write zeros to the new allocated block */
                block_write(fs_device, ind_disk_inode->sectors[i], zeros);
            }

            /* Write the indirect block to the disk */
            block_write(fs_device, disk_inode->indirect, ind_disk_inode);
            free(ind_disk_inode);

            /* Find the number of remaining sectors to allocate */
            sectors -= num_blocks;
            /* If we still have sectors left to allocate, begin allocating in the
             * double indirect block */
            if (sectors > 0)
            {
                /* Allocate double indirect block */
                struct indirect_inode_disk * d_ind_disk_inode = NULL;
                d_ind_disk_inode = calloc(1, sizeof * d_ind_disk_inode);
                
                /* Allocate a free block by finding the first free block. If this 
                 * fails, there is no more space. Write the inode to disk and
                 * return false */
                if (!free_map_allocate(1, &disk_inode->double_indirect))
                {
                    free(d_ind_disk_inode);
                    block_write(fs_device, sector, disk_inode);
                    free(disk_inode);
                    return success;
                }

                /* Find the number of indirect blocks needed */
                int num_indirect_blocks = DIV_ROUND_UP(sectors, NUM_ENTRIES);

                /* The number of indirect blocks to allocate is the minimum
                 * of the number of blocks needed and the number of blocks
                 * available */
                num_indirect_blocks = min(num_indirect_blocks, NUM_ENTRIES);

                /* Allocate indirect blocks */
                for (i = 0; i < num_indirect_blocks; i++)
                {

                    struct indirect_inode_disk * ind_disk_inode = NULL;
                    ind_disk_inode = calloc(1, sizeof * ind_disk_inode);

                    /* Allocate a free block by finding the first free block. If this 
                     * fails, there is no more space. Write the double indirect
                     * block and inode to disk and return false */
                    if (!free_map_allocate(1, &d_ind_disk_inode->sectors[i]))
                    {
                        free(ind_disk_inode);
                        block_write(fs_device, disk_inode->double_indirect, d_ind_disk_inode);
                        free(d_ind_disk_inode);
                        block_write(fs_device, sector, disk_inode);
                        free(disk_inode);
                        return success;
                    }

                    /* The number of blocks to allocate in the indirect block */
                    int num_blocks = min(sectors, NUM_ENTRIES);

                    /* Allocate direct blocks */
                    int j;
                    for (j = 0; j < num_blocks; j++)
                    {

                        /* Allocate a free block by finding the first free block. If this 
                         * fails, there is no more space. Write the double indirect
                         * block, indirect block, and inode to disk and return false */
                        if (!free_map_allocate(1, &ind_disk_inode->sectors[j]))
                        {
                            block_write(fs_device, d_ind_disk_inode->sectors[i], ind_disk_inode);
                            free(ind_disk_inode);
                            block_write(fs_device, disk_inode->double_indirect, d_ind_disk_inode);
                            free(d_ind_disk_inode);
                            block_write(fs_device, sector, disk_inode);
                            free(disk_inode);
                            return success;
                        }

                        /* Write zeros to the new allocated block */
                        block_write(fs_device, ind_disk_inode->sectors[j], zeros);
                    }

                    /* The number of remaining sectors */
                    sectors -= num_blocks;
                    
                    /* Write the indirect block to the disk */
                    block_write(fs_device, d_ind_disk_inode->sectors[i], ind_disk_inode);
                }

                /* Write the double indirect block to the disk */
                block_write(fs_device, disk_inode->double_indirect, d_ind_disk_inode);
                free(d_ind_disk_inode);

                /* If there are remaining sectors, there are no more blocks
                 * left. Write the inode to disk and return false */
                if (sectors > 0)
                {
                   block_write(fs_device, sector, disk_inode);
                   free(disk_inode);
                   return success;
                }
            }
        }

        /* We have successfully created the file */
        /* Write the inode to disk */
        block_write(fs_device, sector, disk_inode);
        success = true; 
        
        free(disk_inode);
    }
    return success;
}

/*! Reads an inode from SECTOR
    and returns a `struct inode' that contains it.
    Returns a null pointer if memory allocation fails. */
struct inode * inode_open(block_sector_t sector) {
    struct list_elem *e;
    struct inode *inode;


    
    /* Check whether this inode is already open. */
    for (e = list_begin(&open_inodes); e != list_end(&open_inodes);
         e = list_next(e)) {
/*    printf("FOR INODE_OPEN\n");
*/        inode = list_entry(e, struct inode, elem);
/*    printf("INODE_OPEN2 inode->sector = %d, sector = %d\n", inode->sector, sector);
*/        if (inode->sector == sector) {
/*    printf("INODE_OPEN3\n");
*/            inode_reopen(inode);
/*    printf("INODE_OPEN4\n");
*/            return inode; 
        }
    }
/*    printf("INODE_OPEN\n");
*/
    /* Allocate memory. */
    inode = malloc(sizeof *inode);
    if (inode == NULL)
        return NULL;

    /* Initialize. */
    list_push_front(&open_inodes, &inode->elem);
    inode->sector = sector;
    inode->open_cnt = 1;
    inode->deny_write_cnt = 0;
    inode->removed = false;
    lock_init(&inode->extension_lock);

/*    block_read(fs_device, inode->sector, &inode->data);
*/    return inode;
}

/*! Reopens and returns INODE. */
struct inode * inode_reopen(struct inode *inode) {
    if (inode != NULL)
        inode->open_cnt++;
    return inode;
}

/*! Returns INODE's inode number. */
block_sector_t inode_get_inumber(const struct inode *inode) {
    return inode->sector;
}

/*! Closes INODE and writes it to disk.
    If this was the last reference to INODE, frees its memory.
    If INODE was also a removed inode, frees its blocks. */
void inode_close(struct inode *inode) {
    /* Ignore null pointer. */
    if (inode == NULL)
        return;

    /* Release resources if this was the last opener. */
    if (--inode->open_cnt == 0) {
        /* Remove from inode list and release lock. */
        list_remove(&inode->elem);
 
        /* Deallocate blocks if removed. */
        if (inode->removed) {
            cache_sector * sector = cache_read_sector(inode->sector);
            struct inode_disk * data = (struct inode_disk *) sector->data;
            /*cache_sector sector = cache_read_sector(inode->sector);*/
            /*struct inode_disk *data = (struct inode_disk *) sector.data;*/
            
            size_t length = data->length;
            int i;
            for (i = 0; i < length; i += BLOCK_SECTOR_SIZE)
            {
                block_sector_t block_id = byte_to_sector(inode, i);
                free_map_release(block_id, 1);
            }
            free_map_release(inode->sector, 1);

            /* Done reading inode->sector. */
            done_read(&sector->rw);
        }

        free(inode); 
    }
}

/*! Marks INODE to be deleted when it is closed by the last caller who
    has it open. */
void inode_remove(struct inode *inode) {
    ASSERT(inode != NULL);
    inode->removed = true;
}

/*! Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t inode_read_at(struct inode *inode, void *buffer_, off_t size, off_t offset) {
    uint8_t *buffer = buffer_;
    off_t bytes_read = 0;
/*    uint8_t *bounce = NULL;
*/
/*    printf("Reading %d bytes of data\n", size);
*/
    while (size > 0) {
        /* Disk sector to read, starting byte offset within sector. */
        block_sector_t sector_idx = byte_to_sector (inode, offset);
/*        printf("size = %d, offset = %d, bytes_read = %d\n", size, offset, bytes_read);
        printf("Reading sector %d\n", sector_idx);
*/        int sector_ofs = offset % BLOCK_SECTOR_SIZE;

        /* Bytes left in inode, bytes left in sector, lesser of the two. */
        off_t inode_left = inode_length(inode) - offset;
        int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
        int min_left = inode_left < sector_left ? inode_left : sector_left;

        /* Number of bytes to actually copy out of this sector. */
        int chunk_size = size < min_left ? size : min_left;
/*        printf("chunk_size = %d\n", chunk_size);
*/        if (chunk_size <= 0)
            break;


        /* If there are things to read, let's get it into our cache. */
        cache_sector *sector = cache_read_sector(sector_idx);
        /*cache_sector sector = cache_read_sector(sector_idx);*/

        /* Read from cache into the buffer */
        memcpy(buffer + bytes_read, sector->data + sector_ofs, chunk_size);
        /* Done reading sector. */
        done_read(&sector->rw);

/*         if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE) { 
*/            /* Read full sector directly into caller's buffer. */
         /*    block_read (fs_device, sector_idx, buffer + bytes_read);
        }
        else {*/ 
            /* Read sector into bounce buffer, then partially copy
               into caller's buffer. */
            /*if (bounce == NULL) {
                bounce = malloc(BLOCK_SECTOR_SIZE);
                if (bounce == NULL)
                    break;
            }
            block_read(fs_device, sector_idx, bounce);
            memcpy(buffer + bytes_read, bounce + sector_ofs, chunk_size);
        } 
      */
        /* Advance. */
        size -= chunk_size;
        offset += chunk_size;
        bytes_read += chunk_size;
/*        printf("%d bytes left to read in\n", size);*/
    }
/*    free(bounce);*/

    /* TODO: Possibly do read ahead here. */

    return bytes_read;
}

bool inode_extend(struct inode *inode, off_t offset) {

    cache_sector * sector = cache_write_sector(inode->sector);
    struct inode_disk * disk_inode = (struct inode_disk *) sector->data;
    int length = disk_inode->length;
    length = DIV_ROUND_UP(length, NUM_BYTES_PER_SECTOR) * NUM_BYTES_PER_SECTOR;

    bool success = false;

    if (offset > length)
    {
        int num_bytes = offset - length;
        int num_blocks = DIV_ROUND_UP(num_bytes, NUM_BYTES_PER_SECTOR);
        int last_block = length / NUM_BYTES_PER_SECTOR;
        bool create_indirect = false;
        while (last_block < NUM_DIRECT)
        {
            create_indirect = true;
            if (num_blocks <= 0 || !free_map_allocate(1, &disk_inode->direct[last_block]))
            {
                done_write(&sector->rw);
                return num_blocks <= 0;
            }

            last_block++;
            num_blocks--;
        }

        if (num_blocks <= 0)
        {
            success = true;
            done_write(&sector->rw);
            return success;
        }


        if (create_indirect)
        {
            struct indirect_inode_disk * ind_disk_inode = NULL;
            ind_disk_inode = calloc(1, sizeof * ind_disk_inode);
            if(!free_map_allocate(1, &disk_inode->indirect))
            {
                done_write(&sector->rw);
                return success;
            }
            block_write(fs_device, disk_inode->indirect, ind_disk_inode);
            free(ind_disk_inode);
        }

        bool create_double_indirect = false;

        cache_sector * indirect_sector = cache_write_sector(disk_inode->indirect);
        struct indirect_inode_disk * indirect_data = (struct indirect_inode_disk *) indirect_sector->data;

        if (last_block < NUM_DIRECT + NUM_ENTRIES)
        {
            create_double_indirect = true;
            int indirect_index = last_block - NUM_DIRECT;
            int i;
            for (i = indirect_index; i < NUM_ENTRIES; i++)
            {
                if (num_blocks <= 0 || !free_map_allocate(1, &indirect_data->sectors[i]))
                {
                    done_write(&indirect_sector->rw);
                    done_write(&sector->rw);
                    return num_blocks <= 0;
                }
                last_block++;
                num_blocks--;
                
            }
        }

        done_write(&indirect_sector->rw);
        if (num_blocks <= 0)
        {
            done_write(&sector->rw);
            success = true;
            return success;
        }

        if (create_double_indirect)
        {
            /*struct indirect_inode_disk * d_ind_disk_inode = NULL;*/
            /*d_ind_disk_inode = calloc(1, sizeof * d_ind_disk_inode);*/
            if(!free_map_allocate(1, &disk_inode->double_indirect))
            {
                done_write(&sector->rw);
                return success;
            }

            cache_sector * d_indirect_sector = cache_write_sector(disk_inode->double_indirect);
            struct indirect_inode_disk * d_ind_disk_inode = (struct indirect_inode_disk *) d_indirect_sector->data;
            if(!free_map_allocate(1, &d_ind_disk_inode->sectors[0]))
            {
                done_write(&d_indirect_sector->rw);
                done_write(&sector->rw);
                return success;
            }

            done_write(&d_indirect_sector->rw);
        }

        if (last_block < NUM_DIRECT + NUM_ENTRIES + NUM_ENTRIES * NUM_ENTRIES)
        {
            int indirect_index = (last_block - NUM_DIRECT - NUM_ENTRIES) / NUM_ENTRIES;
            cache_sector * d_indirect_sector = cache_write_sector(disk_inode->double_indirect);
            struct indirect_inode_disk * double_indirect_data = (struct indirect_inode_disk *) d_indirect_sector->data;
            while (indirect_index < NUM_ENTRIES)
            {
                cache_sector * indirect_sector = cache_write_sector(double_indirect_data->sectors[indirect_index]);
                struct indirect_inode_disk * indirect_data = (struct indirect_inode_disk *) indirect_sector->data;
                int direct_index = (last_block - NUM_DIRECT - indirect_index * NUM_ENTRIES);
                int i;
                for (i = direct_index; i < NUM_ENTRIES; i++)
                {
                    if (num_blocks <= 0 || !free_map_allocate(1, &indirect_data->sectors[i]))
                    {
                        done_write(&d_indirect_sector->rw);
                        done_write(&indirect_sector->rw);
                        done_write(&sector->rw);
                        return num_blocks <= 0;
                    }
                    last_block++;
                    num_blocks--;
                }

                done_write(&indirect_sector->rw);
                if (num_blocks <= 0)
                {
                        done_write(&d_indirect_sector->rw);
                        done_write(&sector->rw);
                        success = true;
                        return success;
                }

                indirect_index++;
                if (indirect_index < NUM_ENTRIES)
                {
                    if(!free_map_allocate(1, &double_indirect_data->sectors[indirect_index]))
                    {
                        done_write(&d_indirect_sector->rw);
                        done_write(&sector->rw);
                        return success;
                    }
                }
            }
        }
    }

    done_write(&sector->rw);
    return success;
}

/*! Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
    Returns the number of bytes actually written, which may be
    less than SIZE if end of file is reached or an error occurs.
    (Normally a write at end of file would extend the inode, but
    growth is not yet implemented.) */
off_t inode_write_at(struct inode *inode, const void *buffer_, off_t size, off_t offset) {
    const uint8_t *buffer = buffer_;
    off_t bytes_written = 0;
/*    uint8_t *bounce = NULL;
*/
    if (inode->deny_write_cnt)
        return 0;

    int disk_inode_length = inode_length(inode);
    int length = DIV_ROUND_UP(disk_inode_length, NUM_BYTES_PER_SECTOR) * NUM_BYTES_PER_SECTOR;

    if (offset + size > disk_inode_length)
    {
        disk_inode_length = offset + size;
        if (offset + size > length)
        {
            lock_acquire(&inode->extension_lock);
            /*disk_inode_length = DIV_ROUND_UP(offset + size, NUM_BYTES_PER_SECTOR) * NUM_BYTES_PER_SECTOR;*/
            if (offset + size > length) {
                inode_extend(inode, offset + size);
                cache_sector * sector = cache_write_sector(inode->sector);
                struct inode_disk * disk_inode = (struct inode_disk *) sector->data;
                disk_inode->length = disk_inode_length;
                done_write(&sector->rw);
            }
            lock_release(&inode->extension_lock);
        }
        else
        {
            lock_acquire(&inode->extension_lock);
            cache_sector * sector = cache_write_sector(inode->sector);
            struct inode_disk * disk_inode = (struct inode_disk *) sector->data;
            disk_inode->length = disk_inode_length;
            done_write(&sector->rw);
            lock_release(&inode->extension_lock);
        }
    }

    /*cache_sector * sector = cache_write_sector(inode->sector);*/
    /*struct inode_disk * disk_inode = (struct inode_disk *) sector->data;*/
    /*done_write(&sector->rw);*/


    while (size > 0) {
        /* Sector to write, starting byte offset within sector. */
        block_sector_t sector_idx = byte_to_sector(inode, offset);
        int sector_ofs = offset % BLOCK_SECTOR_SIZE;

        /* Bytes left in inode, bytes left in sector, lesser of the two. */
        off_t inode_left = inode_length(inode) - offset;
        int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
        int min_left = inode_left < sector_left ? inode_left : sector_left;

        /* Number of bytes to actually write into this sector. */
        int chunk_size = size < sector_left ? size : sector_left;
        if (chunk_size <= 0)
            break;

        /* If there are things to write, let's get it into our cache. */
        cache_sector *sector = cache_write_sector(sector_idx);
        /*printf("Got the sector\n");*/
        /*cache_sector sector = cache_write_sector(sector_idx);*/

        /* Write from cache into the buffer */
        memcpy(sector->data + sector_ofs, buffer + bytes_written, chunk_size);
        sector->dirty = true;
        done_write(&sector->rw);
        /*printf("Wrote to file\n");*/

/*        if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE) {
*/            /* Write full sector directly to disk. */
/*            block_write(fs_device, sector_idx, buffer + bytes_written);
        }
        else {
*/            /* We need a bounce buffer. */
/*            if (bounce == NULL) {
                bounce = malloc(BLOCK_SECTOR_SIZE);
                if (bounce == NULL)
                    break;
            }
*/
            /* If the sector contains data before or after the chunk
               we're writing, then we need to read in the sector
               first.  Otherwise we start with a sector of all zeros. */

/*            if (sector_ofs > 0 || chunk_size < sector_left) 
                block_read(fs_device, sector_idx, bounce);
            else
                memset (bounce, 0, BLOCK_SECTOR_SIZE);

            memcpy(bounce + sector_ofs, buffer + bytes_written, chunk_size);
            block_write(fs_device, sector_idx, bounce);
        }
*/
        /* Advance. */
        size -= chunk_size;
        offset += chunk_size;
        bytes_written += chunk_size;
    }
/*    free(bounce);
*/
    /*sector = cache_write_sector(inode->sector);*/
    /*disk_inode = (struct inode_disk *) sector->data;*/
    /*done_write(&sector->rw);*/
    return bytes_written;
}

/*! Disables writes to INODE.
    May be called at most once per inode opener. */
void inode_deny_write (struct inode *inode) {
    inode->deny_write_cnt++;
    ASSERT(inode->deny_write_cnt <= inode->open_cnt);
}

/*! Re-enables writes to INODE.
    Must be called once by each inode opener who has called
    inode_deny_write() on the inode, before closing the inode. */
void inode_allow_write (struct inode *inode) {
    ASSERT(inode->deny_write_cnt > 0);
    ASSERT(inode->deny_write_cnt <= inode->open_cnt);
    inode->deny_write_cnt--;
}

/*! Returns the length, in bytes, of INODE's data. */
off_t inode_length(const struct inode *inode) {
    cache_sector *sector = cache_read_sector(inode->sector);
    struct inode_disk *data = (struct inode_disk *) sector->data;
    off_t result = data->length;
    done_read(&sector->rw);

    return result;
}


bool inode_is_dir(struct thread * t, int fd) {
/* Loop through all the threads out there looking for the fd. */
    // struct list_elem *e;
    // struct list all_list = get_all_list();
    // for (e = list_begin(&all_list); e != list_end(&all_list);
    //      e = list_next(e)) {
    //     struct thread *t = list_entry(e, struct thread, allelem);
    //     if (fd >= 2 && fd < NUM_FILES && t->files[fd - 2] != NULL)
    //     {
            struct inode *inode = file_get_inode(t->files[fd - 2]);
            // printf("%d", inode->sector);
            cache_sector * sector = cache_read_sector(inode->sector);
            struct inode_disk * disk_inode = (struct inode_disk *) sector->data;
            bool result = disk_inode->isDirectory;
            done_read(&sector->rw);
            return result;
            /*cache_sector *sector = cache_read_sector(inode->sector);
            struct inode_disk *data = (struct inode_disk *) sector->data;
            bool result = data->isDirectory;
            done_read(&sector->rw);
            return result;*/
    //     }
    // }

    // return false;
}


block_sector_t inode_get_inumber_from_fd(struct thread * t, int fd) {
    // struct list_elem *e;
    // struct list all_list = get_all_list();
    // for (e = list_begin(&all_list); e != list_end(&all_list);
    //      e = list_next(e)) {
    //     struct thread *t = list_entry(e, struct thread, allelem);
    //     if (fd >= 2 && fd < NUM_FILES && t->files[fd - 2] != NULL)
    //     {
            struct inode *inode = file_get_inode(t->files[fd - 2]);
            return inode->sector;
            // return file_get_inode(t->files[fd - 2])->sector;
    //     }
    // }
    // return -1;
}



bool inode_readdir(struct thread * t, int fd, char * name) {
    // struct list_elem *e;
    // struct list all_list = get_all_list();
    // for (e = list_begin(&all_list); e != list_end(&all_list);
    //      e = list_next(e)) {
    //     struct thread *t = list_entry(e, struct thread, allelem);
    //     if (fd >= 2 && fd < NUM_FILES && t->files[fd - 2] != NULL)
    //     {
    // printf("BEFORE TEH IF name = %s\n", name);
            if(!strcmp(name, ".") || !strcmp(name, "..")) {
                return false;
            }
            // printf("AFTER THE IF\n");
            struct inode *inode = file_get_inode(t->files[fd - 2]);
            struct dir *dir = dir_open(inode);
            bool success = dir_readdir(dir, name);
            /* Must check if name is . or .. */
            return success;
            // return file_get_inode(t->files[fd - 2])->sector;
    //     }
    // }
    // return false;
}


void inode_set_dir(block_sector_t sector_id, bool isDir) {
    cache_sector * sector = cache_write_sector(sector_id);
    struct inode_disk * disk_inode = (struct inode_disk *) sector->data;
    disk_inode->isDirectory = isDir;
    done_write(&sector->rw);
}


bool inode_is_removed(struct inode * inode) {
    return inode->removed;
}
