#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "filesys/cache.h"
#include <stdio.h>

/*! Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

/* Constants for number of entries */
#define NUM_DIRECT 123 
#define NUM_BYTES_PER_SECTOR 512
#define NUM_ENTRIES NUM_BYTES_PER_SECTOR / 4
#define METADATA_BYTES (NUM_ENTRIES - NUM_DIRECT) * 4

/*! On-disk inode.
    Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk {
    block_sector_t start;               /*!< First data sector. */
    off_t length;                       /*!< File size in bytes. */
    unsigned magic;                     /*!< Magic number. */
    block_sector_t direct[NUM_DIRECT];
    block_sector_t indirect;
    block_sector_t double_indirect;
};

struct indirect_inode_disk {
    block_sector_t sectors[NUM_ENTRIES];
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
    /*struct inode_disk data;*/             /*!< Inode content. */
};

static inline int min(int a, int b)
{
    return a < b ? a : b;
}

/*! Returns the block device sector that contains byte offset POS
    within INODE.
    Returns -1 if INODE does not contain data for a byte at offset
    POS. */
static block_sector_t byte_to_sector(const struct inode *inode, off_t pos) {
    /*printf("Byte to sector: %d\n", pos);*/
    ASSERT(inode != NULL);

    /* Value to return. */
    block_sector_t result;

    /* Start reading inode->sector */
    /*printf("Reading cache\n");*/
    /*printf("Inode sector: %d\n", inode->sector);*/
    int cache_ind = cache_get_sector(inode->sector);
    struct inode_disk * data = (struct inode_disk *) cache[cache_ind].data;
    /*cache_sector sector = cache_read_sector(inode->sector);*/
    /*struct inode_disk * data = (struct inode_disk *) sector.data;*/

    /*if (pos < METADATA_BYTES)*/
    /*{*/
        /*printf("Error in byte_to_sector, pos < metadata\n");*/
        /*return -1;*/
    /*}*/
    /*pos -= METADATA_BYTES;*/
    /*printf("pos -= metadata: %d\n", pos);*/
    /*printf("File length: %d\n", data->length);*/
    if (pos < data->length)
    {
        /*printf("Looking in direct blocks\n");*/
        int block_index = pos / NUM_BYTES_PER_SECTOR;
        /*printf("Block index: %d\n", block_index);*/
        if (block_index < NUM_DIRECT)
        {
            /*printf("Found in direct blocks\n");*/
            result = data->direct[block_index];
            /* Done reading inode->sector. */
            //done_read(&sector.rw);
            /*printf("Done reading sector\n");*/
            /*printf("Sector is %d\n", result);*/
            return result;
        }

        else
        {
            /*printf("Looking in indirect blocks\n");*/
            block_index -= NUM_DIRECT;
            /*printf("Block index: %d\n", block_index);*/
            if (block_index < NUM_ENTRIES)
            {
                /*printf("Found in indirect blocks\n");*/
                int cache_ind = cache_get_sector(data->indirect);
                struct indirect_inode_disk * indirect_data = (struct indirect_inode_disk *) cache[cache_ind].data;
                /*cache_sector indirect_sector = cache_read_sector(data->indirect);*/
                /*struct indirect_inode_disk * indirect_data = (struct indirect_inode_disk *) indirect_sector.data;*/
                result = indirect_data->sectors[block_index];
                /* Done reading indirect data. */
                //done_read(&indirect_sector.rw);
                /*printf("Sector is %d\n", result);*/
                return result;
            }

            else
            {
                /*printf("Looking in double indirect blocks\n");*/
                block_index -= NUM_ENTRIES;
                /*printf("Block index: %d\n", block_index);*/
                if (block_index < NUM_ENTRIES * NUM_ENTRIES)
                {
                    /*printf("Found in double indirect blocks\n");*/
                    int indirect_index = block_index / NUM_ENTRIES;
                    /*printf("Indirect index: %d\n", indirect_index);*/
                    int cache_ind = cache_get_sector(data->double_indirect);
                    struct indirect_inode_disk * double_indirect_data = (struct inode_disk *) cache[cache_ind].data;
                    /*cache_sector double_indirect_sector = cache_read_sector(data->double_indirect);*/
                    /*struct indirect_inode_disk * double_indirect_data = (struct indirect_inode_disk *) double_indirect_sector.data;*/
                    block_sector_t indirect_sector_id = double_indirect_data->sectors[indirect_index];
                    /* Done reading double indirect data. */
                    //done_read(&double_indirect_sector.rw);
                    int direct_index = block_index % NUM_ENTRIES;
                    cache_ind = cache_get_sector(indirect_sector_id);
                    struct indirect_inode_disk * indirect_data = (struct indirect_inode_disk *) cache[cache_ind].data;
                    result = indirect_data->sectors[direct_index];

                    /* Done reading indirect data. */
                    //done_read(&indirect_sector.rw);
                    /*printf("Sector is %d\n", result);*/
                    return result;
                }

                else
                {
                    /*printf("Error in byte_to_sector, offset too big\n");*/
                    return -1;
                }
            }
        }
    }

    else {
        /* Done reading inode->sector. */
        //done_read(&sector.rw);
        /*printf("Error in byte_to_sector, offset > length\n");*/
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
        size_t sectors = bytes_to_sectors(length);
        disk_inode->length = length;
        disk_inode->magic = INODE_MAGIC;
        volatile int num_direct_blocks = min(sectors, NUM_DIRECT);
        int i;
        for (i = 0; i < num_direct_blocks; i++)
        {
            if (!free_map_allocate(1, &disk_inode->direct[i]))
            {
                block_write(fs_device, sector, disk_inode);
                free(disk_inode);
                return success;
            }
            /*printf("Found direct sector %d\n", disk_inode->direct[i])*/
            block_write(fs_device, disk_inode->direct[i], zeros);
        }
        sectors -= num_direct_blocks;
        if (sectors > 0)
        {
            struct indirect_inode_disk * ind_disk_inode = NULL;
            ind_disk_inode = calloc(1, sizeof * ind_disk_inode);
            if (!free_map_allocate(1, &disk_inode->indirect))
            {
                free(ind_disk_inode);
                block_write(fs_device, sector, disk_inode);
                free(disk_inode);
                return success;
            }
            int num_blocks = min(sectors, NUM_ENTRIES);
            for (i = 0; i < num_blocks; i++)
            {
                if (!free_map_allocate(1, &ind_disk_inode->sectors[i]))
                {
                    block_write(fs_device, disk_inode->indirect, ind_disk_inode);
                    free(ind_disk_inode);
                    block_write(fs_device, sector, disk_inode);
                    free(disk_inode);
                    return success;
                }
                static char zeros[BLOCK_SECTOR_SIZE];
                block_write(fs_device, ind_disk_inode->sectors[i], zeros);
            }
            block_write(fs_device, disk_inode->indirect, ind_disk_inode);
            free(ind_disk_inode);
            sectors -= num_blocks;
            if (sectors > 0)
            {
                struct indirect_inode_disk * d_ind_disk_inode = NULL;
                d_ind_disk_inode = calloc(1, sizeof * d_ind_disk_inode);
                if (!free_map_allocate(1, &disk_inode->double_indirect))
                {
                    block_write(fs_device, disk_inode->double_indirect, d_ind_disk_inode);
                    free(d_ind_disk_inode);
                    block_write(fs_device, sector, disk_inode);
                    free(disk_inode);
                    return success;
                }
                int num_indirect_blocks = DIV_ROUND_UP(sectors, NUM_ENTRIES);
                num_indirect_blocks = min(num_indirect_blocks, NUM_ENTRIES);
                for (i = 0; i < num_indirect_blocks; i++)
                {
                    struct indirect_inode_disk * ind_disk_inode = NULL;
                    ind_disk_inode = calloc(1, sizeof * ind_disk_inode);
                    if (!free_map_allocate(1, &d_ind_disk_inode->sectors[i]))
                    {
                        free(ind_disk_inode);
                        block_write(fs_device, disk_inode->double_indirect, d_ind_disk_inode);
                        free(d_ind_disk_inode);
                        block_write(fs_device, sector, disk_inode);
                        free(disk_inode);
                        return success;
                    }
                    int num_blocks = min(sectors, NUM_ENTRIES);
                    int j;
                    for (j = 0; j < num_blocks; j++)
                    {
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
                        block_write(fs_device, ind_disk_inode->sectors[j], zeros);
                    }
                    sectors -= num_blocks;
                    block_write(fs_device, d_ind_disk_inode->sectors[i], ind_disk_inode);
                }
                block_write(fs_device, disk_inode->double_indirect, d_ind_disk_inode);
                free(d_ind_disk_inode);

                if (sectors > 0)
                {
                   block_write(fs_device, sector, disk_inode);
                   free(disk_inode);
                   return success;
                }
            }
        }

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
        inode = list_entry(e, struct inode, elem);
        if (inode->sector == sector) {
            inode_reopen(inode);
            return inode; 
        }
    }

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
            int cache_ind = cache_get_sector(inode->sector);
            struct inode_disk * data = (struct inode_disk *) cache[cache_ind].data;
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
            //done_read(&sector.rw);
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
        int cache_ind = cache_get_sector(sector_idx);
        /*cache_sector sector = cache_read_sector(sector_idx);*/

        /* Read from cache into the buffer */
        memcpy(buffer + bytes_read, cache[cache_ind].data + sector_ofs, chunk_size);
        /* Done reading sector. */
        //done_read(&sector.rw);

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

    int cache_ind = cache_get_sector(inode->sector);
    struct inode_disk * disk_inode = (struct inode_disk *) cache[cache_ind].data;
    int length = disk_inode->length;

    bool success = false;

    if (offset >= length)
    {
        int num_bytes = length - offset;
        int num_blocks = num_bytes / NUM_BYTES_PER_SECTOR;
        int last_block = length / NUM_BYTES_PER_SECTOR;
        bool create_indirect = false;
        while (last_block < NUM_DIRECT)
        {
            create_indirect = true;
            if (num_blocks <= 0 || !free_map_allocate(1, &disk_inode->direct[last_block]))
            {
                block_write(fs_device, inode->sector, disk_inode);
                return num_blocks <= 0;
            }

            last_block++;
            num_blocks--;
        }

        if (num_blocks <= 0)
        {
            block_write(fs_device, inode->sector, disk_inode);
            success = true;
            return success;
        }


        if (create_indirect)
        {
            struct indirect_inode_disk * ind_disk_inode = NULL;
            ind_disk_inode = calloc(1, sizeof * ind_disk_inode);
            if(!free_map_allocate(1, &disk_inode->indirect))
            {
                block_write(fs_device, inode->sector, disk_inode);
                return success;
            }
            block_write(fs_device, disk_inode->indirect, ind_disk_inode);
            free(ind_disk_inode);
        }

        bool create_double_indirect = false;

        int cache_ind = cache_get_sector(disk_inode->indirect);
        struct indirect_inode_disk * indirect_data = (struct indirect_inode_disk *) cache[cache_ind].data;

        if (last_block < NUM_DIRECT + NUM_ENTRIES)
        {
            create_double_indirect = true;
            int indirect_index = last_block - NUM_DIRECT;
            int i;
            for (i = indirect_index; i < NUM_ENTRIES; i++)
            {
                if (num_blocks <= 0 || !free_map_allocate(1, &indirect_data->sectors[i]))
                {
                    block_write(fs_device, disk_inode->indirect, indirect_data);
                    block_write(fs_device, inode->sector, disk_inode);
                    return num_blocks <= 0;
                }
                last_block++;
                num_blocks--;
                
            }
        }

        block_write(fs_device, disk_inode->indirect, indirect_data);
        if (num_blocks <= 0)
        {
            block_write(fs_device, inode->sector, disk_inode);
            success = true;
            return success;
        }

        if (create_double_indirect)
        {
            struct indirect_inode_disk * d_ind_disk_inode = NULL;
            d_ind_disk_inode = calloc(1, sizeof * d_ind_disk_inode);
            if(!free_map_allocate(1, &disk_inode->double_indirect))
            {
                block_write(fs_device, inode->sector, disk_inode);
                return success;
            }

            struct indirect_inode_disk * ind_disk_inode = NULL;
            ind_disk_inode = calloc(1, sizeof * ind_disk_inode);
            if(!free_map_allocate(1, &d_ind_disk_inode->sectors[0]))
            {
                block_write(fs_device, disk_inode->double_indirect, d_ind_disk_inode);
                block_write(fs_device, inode->sector, disk_inode);
                return success;
            }

            block_write(fs_device, d_ind_disk_inode->sectors[0], ind_disk_inode);
            block_write(fs_device, disk_inode->double_indirect, d_ind_disk_inode);
            free(ind_disk_inode);
            free(d_ind_disk_inode);
        }

        if (last_block < NUM_DIRECT + NUM_ENTRIES + NUM_ENTRIES * NUM_ENTRIES)
        {
            int indirect_index = (last_block - NUM_DIRECT - NUM_ENTRIES) / NUM_ENTRIES;
            int cache_ind = cache_get_sector(disk_inode->double_indirect);
            struct indirect_inode_disk * double_indirect_data = (struct indirect_inode_disk *) cache[cache_ind].data;
            while (indirect_index < NUM_ENTRIES)
            {
                cache_ind = cache_get_sector(double_indirect_data->sectors[indirect_index]);
                struct indirect_inode_disk * indirect_data = (struct indirect_inode_disk *) cache[cache_ind].data;
                int direct_index = (last_block - NUM_DIRECT - indirect_index * NUM_ENTRIES);
                int i;
                for (i = direct_index; i < NUM_ENTRIES; i++)
                {
                    if (num_blocks <= 0 || !free_map_allocate(1, &indirect_data->sectors[i]))
                    {
                        block_write(fs_device, double_indirect_data->sectors[indirect_index], indirect_data);
                        block_write(fs_device, disk_inode->double_indirect, double_indirect_data);
                        block_write(fs_device, inode->sector, disk_inode);
                        return num_blocks <= 0;
                    }
                    last_block++;
                    num_blocks--;
                }

                block_write(fs_device, double_indirect_data->sectors[indirect_index], indirect_data);
                if (num_blocks <= 0)
                {
                        block_write(fs_device, disk_inode->double_indirect, double_indirect_data);
                        block_write(fs_device, inode->sector, disk_inode);
                        success = true;
                        return success;
                }

                indirect_index++;
                if (indirect_index < NUM_ENTRIES)
                {
                    struct indirect_inode_disk * ind_disk_inode = NULL;
                    ind_disk_inode = calloc(1, sizeof * ind_disk_inode);
                    if(!free_map_allocate(1, &double_indirect_data->sectors[indirect_index]))
                    {
                        block_write(fs_device, disk_inode->double_indirect, double_indirect_data);
                        block_write(fs_device, inode->sector, disk_inode);
                        return success;
                    }
                }
            }
        }
    }

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

    int cache_ind = cache_get_sector(inode->sector);
    struct inode_disk * disk_inode = (struct inode_disk *) cache[cache_ind].data;

    if (offset >= disk_inode->length)
    {
        if (inode_extend(inode, offset))
        {
            disk_inode->length += offset - disk_inode->length;
        }
    }


    while (size > 0) {
        /* Sector to write, starting byte offset within sector. */
        block_sector_t sector_idx = byte_to_sector(inode, offset);
        int sector_ofs = offset % BLOCK_SECTOR_SIZE;

        /* Bytes left in inode, bytes left in sector, lesser of the two. */
        off_t inode_left = inode_length(inode) - offset;
        int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
        int min_left = inode_left < sector_left ? inode_left : sector_left;

        /* Number of bytes to actually write into this sector. */
        int chunk_size = size < min_left ? size : min_left;
        if (chunk_size <= 0)
            break;

        /* If there are things to write, let's get it into our cache. */
        int cache_ind = cache_get_sector(sector_idx);
        /*cache_sector sector = cache_write_sector(sector_idx);*/

        /* Write from cache into the buffer */
        memcpy(cache[cache_ind].data + sector_ofs, buffer + bytes_written, chunk_size);
        cache[cache_ind].dirty = true;

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
    int cache_ind = cache_get_sector(inode->sector);
    struct inode_disk * data = (struct inode_disk *) cache[cache_ind].data;
    /*cache_sector sector = cache_read_sector(inode->sector);*/
    /*struct inode_disk *data = (struct inode_disk *) sector.data;*/
    off_t result = data->length;
    //done_read(&sector.rw);

    return result;
}

