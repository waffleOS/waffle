#include "filesys/cache.h"

/**
 * cache.c
 * Header file for all the caching operations so we can cache stuff from the
 * disk and not have to stuff in from disk every time we need to read or
 * write data.
 */

void cache_init(void) {
	int i;
	for(i = 0; i < CACHE_SIZE; i++) {
		cache[i].used = false;
		cache[i].dirty = false;
		cache[i].accessed = false;
		cache[i].block_id = 0;
		cache_access_count[i] = 0;
	}
	cache_refresh_count = 0;
}


bool cache_is_dirty(block_sector_t block_id) {
	int i;
	for(i = 0; i < CACHE_SIZE; i++) {
		if(cache[i].block_id == block_id) {
			return cache[i].dirty;
		}
	}
	/* Couldn't find cache. */
	return false;
}

void cache_set_dirty(block_sector_t block_id, bool dirty) {
	int i;
	for(i = 0; i < CACHE_SIZE; i++) {
		if(cache[i].block_id == block_id) {
			cache[i].dirty = dirty;
			return;
		}
	}
	
	/* Couldn't find cache. */
}

void cache_write_all_dirty(void) {
	int i;
	for(i = 0; i < CACHE_SIZE; i++) {
		if(cache[i].dirty) {
            block_write(fs_device, cache[i].block_id, cache[i].data);
		}
	}
}

bool cache_is_used(block_sector_t block_id) {
	int i;
	for(i = 0; i < CACHE_SIZE; i++) {
		if(cache[i].block_id == block_id) {
			return cache[i].used;
		}
	}
	return false;
}

/* Returns index of evicted cache_sector. Must write back if dirty. Sets used
 * flag to false. */
int cache_evict(void) {
	/* Current eviction policy: Least frequently used. Will update this
	later if time to something better. */
	int i, evict_ind = 0;
	for(i = 1; i < CACHE_SIZE; i++) {
		if(cache[i].used == false) { /* If sector is unused, use it */
			return i;
		}
		if(cache_access_count[i] < cache_access_count[evict_ind]) {
			evict_ind = i;
		}
	}

	/* Evict the sector at evict_ind */
    block_write(fs_device, cache[evict_ind].block_id, cache[evict_ind].data);
	cache[evict_ind].used = false;
	return evict_ind;
}

/* Gets the index of the cache of the requested sector. If it is not in the 
 * cache, we fetch the sector from disk to cache and insert it into the cache. 
 * Evicts another sector if needed. */
int cache_get_sector(block_sector_t block_id) {
	/* Check if we already have it in the cache */
	int i;
	for(i = 0; i < CACHE_SIZE; i++) {
		if(cache[i].used && cache[i].block_id == block_id) {
			return i;
		}
	}

	/* If we don't have it in the cache, then we have to read it in from
	disk */
	int insert_ind = cache_evict();
    block_read(fs_device, cache[insert_ind].block_id, cache[insert_ind].data);
    cache[insert_ind].used = true;
    cache[insert_ind].dirty = false;
    return insert_ind;
}