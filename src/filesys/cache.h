/**
 * cache.h
 * Header file for all the caching operations so we can cache stuff from the
 * disk and not have to stuff in from disk every time we need to read or
 * write data.
 */

#include <stdbool.h>
#include "devices/block.h"
#include "filesys/filesys.h"


#define CACHE_SIZE 63
#define CACHE_REFRESH_LIMIT 50
#define CACHE_DELAY_MULTIPLIER 10 /* Multiply by CACHE_REFRESH_LIMIT to get how
                                  long we wait until we decay the access count */

typedef struct cache_sector {
	block_sector_t block_id;         /* Sector index contained in cache */
	bool used;                       /* Has this sector been used yet */
	bool dirty;                      /* Has this sector been written to */
	bool accessed;                   /* Has this sector been accessed recently */
	uint8_t data[BLOCK_SECTOR_SIZE]; /* Holds up to BLOCK_SECTOR_SIZE bytes */
} cache_sector;

cache_sector cache[CACHE_SIZE];

/* This array keeps track of the number of times a cache_sector is accessed
 * so we can implement an eviction policy (least frequently used). This must
 * updated when the inode_read_at function is called. */
unsigned int cache_access_count[CACHE_SIZE];

/* This counter keeps track of cache events. Cache events will (probably) be
 * defined as cache accesses, cache evictions, writing to cache, etc. At a 
 * certain count level defined by CACHE_REFRESH_LIMIT, all dirty sectors will
 * be written back to disk as a safety measure against crashes. */
unsigned int cache_refresh_count;

void cache_init(void);
bool cache_is_dirty(block_sector_t block_id);
void cache_set_dirty(block_sector_t block_id, bool dirty);
void cache_write_all_dirty(void);
bool cache_is_used(block_sector_t block_id);

/* Returns index of evicted cache_sector. Must write back if dirty. Sets used
 * flag to false. */
int cache_evict(void);

/* Fetches sector from disk to cache and inserts into cache. Evicts another
 * sector if needed. */
int cache_get_sector(block_sector_t block_id);

void cache_refresh(void);
