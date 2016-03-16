/**
 * cache.h
 * Header file for all the caching operations so we can cache stuff from the
 * disk and not have to stuff in from disk every time we need to read or
 * write data.
 */

#include <stdbool.h>
#include "devices/block.h"
#include "filesys/filesys.h"
#include "threads/synch.h"

#define CACHE_SIZE 63
#define CACHE_REFRESH_LIMIT 50
#define CACHE_DELAY_MULTIPLIER 10 /* Multiply by CACHE_REFRESH_LIMIT to get how
                                  long we wait until we decay the access count */

typedef struct cache_sector {
	block_sector_t block_id;         /* Sector index contained in cache */
	bool used;                       /* Has this sector been used yet */
	bool dirty;                      /* Has this sector been written to */
	bool accessed;                   /* Has this sector been accessed recently */
    struct rw_lock rw;               /* Synchronizes reads and writes. */
	uint8_t data[BLOCK_SECTOR_SIZE]; /* Holds up to BLOCK_SECTOR_SIZE bytes */
} cache_sector;

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
cache_sector cache_read_sector(block_sector_t block_id); 
cache_sector cache_write_sector(block_sector_t block_id);

void cache_refresh(void);
