#include "filesys/cache.h"
#include <stdio.h>
#include "devices/timer.h"
#include "threads/synch.h"

/**
 * cache.c
 * Header file for all the caching operations so we can cache stuff from the
 * disk and not have to stuff in from disk every time we need to read or
 * write data.
 */
static cache_sector cache[CACHE_SIZE];
/* Guards access to the cache array. */
static struct semaphore cache_sem;


/* This array keeps track of the number of times a cache_sector is accessed
 * so we can implement an eviction policy (least frequently used). This must
 * updated when the inode_read_at function is called. */
static unsigned int cache_access_count[CACHE_SIZE];

/* This counter keeps track of cache events. Cache events will (probably) be
 * defined as cache accesses, cache evictions, writing to cache, etc. At a 
 * certain count level defined by CACHE_REFRESH_LIMIT, all dirty sectors will
 * be written back to disk as a safety measure against crashes. */
static unsigned int cache_refresh_count;

/* Cache module internal function prototypes. */
static int cache_get_sector(block_sector_t block_id);
static int cache_sync_sector(block_sector_t block_id, bool write);

void cache_init(void) {
	int i;
    
    sema_init(&cache_sem, 1);

	for(i = 0; i < CACHE_SIZE; i++) {
		cache[i].used = false;
		cache[i].dirty = false;
		cache[i].accessed = false;
		cache[i].block_id = 0;
        rw_lock_init(&cache[i].rw);
        cache[i].rw.id = i;
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
		cache[i].dirty = false;
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
/*	printf("Entering cache_evict\n");
*/

	/* Current eviction policy: NFU (Not Frequently Used). The initial index
	is a "random" replacement strategy so it doesn't keep thrashing on the
	first index. It isn't perfect, but it should do it most of the time. */
    int i, evict_ind = (int) timer_ticks() % CACHE_SIZE;
    /*int i, evict_ind = 0;*/
	for(i = 0; i < CACHE_SIZE; i++) {
		if(cache[i].used == false) { /* If sector is unused, use it */
			return i;
		}
		if(cache_access_count[i] < cache_access_count[evict_ind]) {
			evict_ind = i;
		}
	}

/*	printf("Evicting %d\n", evict_ind);
*/
	/* Evict the sector at evict_ind */
	if(cache[evict_ind].dirty) {
	    block_write(fs_device, cache[evict_ind].block_id, cache[evict_ind].data);
	}
	cache[evict_ind].used = false;
    
	//printf("evict_ind = %d\n", evict_ind);
	return evict_ind;
}

/* Gets the index of the cache of the requested sector. If it is not in the 
 * cache, we fetch the sector from disk to cache and insert it into the cache. 
 * Evicts another sector if needed. */
int cache_get_sector(block_sector_t block_id) {
	/* Check if we already have it in the cache */
	//printf("cache_get_sector: get sector %d\n", block_id);
	
    sema_down(&cache_sem);
	cache_refresh();
	int i;
	for(i = 0; i < CACHE_SIZE; i++) {
/*		printf("cache_get_sector ind %d blockid = %d\n", i, cache[i].block_id);*/
		if(cache[i].used && cache[i].block_id == block_id) {
			cache[i].accessed = true;
            sema_up(&cache_sem);
/*			printf("cache_get_sector: found in cache at index %d\n", i);
*/			return i;
		}
	}

    sema_up(&cache_sem);

	/* If we don't have it in the cache, then we have to read it in from
	disk */
    int insert_ind;
    cache_sector *sector;
    /* Keep trying to evict until we get the desired block. */
    sema_down(&cache_sem);
    insert_ind = cache_evict();
    sector = &cache[insert_ind];

    /* Eviction involves writing to the cache sector, so wait_write.
     * This lets any active writer or readers to finish what they're doing. */
    //printf("Waiting to evict....\n");
    wait_write(&sector->rw);

    // Crabbing.
    sema_up(&cache_sem);

    /* Eviction involves writing to the cache sector, so
     * wait_write. */
    block_read(fs_device, block_id, sector->data);
    cache[insert_ind].used = true;
    cache[insert_ind].dirty = false;
    cache[insert_ind].accessed = true;
    cache[insert_ind].block_id = block_id;
    cache_access_count[insert_ind] = 0;

    /* Done writing */
    done_write(&sector->rw);

/*	printf("cache_get_sector: read into cache at index %d\n", insert_ind);
*/

    return insert_ind;
}

/* Synchronously gets a sector for reading/writing.
 * Reading blocks if there is a writer already waiting. 
 * Writer blocks unless there is no one using the lock, but
 * gets priority over readers that block after it does. 
 * When the thread is done reading/writing, 
 * it must signal to others using done_read()/done_write() respectively. */
int cache_sync_sector(block_sector_t block_id, bool write) { 
    int cache_ind;
    while (true) { 
        cache_ind = cache_get_sector(block_id);
        cache_sector *sector = &cache[cache_ind];
        
        /* Synchronously acquire sector for reading */
        if (write) { 
            //printf("Waiting to write.\n");
            wait_write(&sector->rw);
        }
        else {
            wait_read(&sector->rw);
        }

        if (sector->block_id == block_id) { 
            //printf("Acquired correct block.\n");
            return cache_ind;
        }

        /* Give up rw_lock if the block changed. */
        if (write) { 
            printf("Failed to write.\n");
            done_write(&sector->rw);
        }
        else { 
            done_read(&sector->rw);
        }
    }
}

/* Gets the cache sector synchronously. Takes care of
 * double checking the sector. Caller must call done_read*/
cache_sector *cache_read_sector(block_sector_t block_id) {
    return &cache[cache_sync_sector(block_id, false)];
}

/* */
cache_sector *cache_write_sector(block_sector_t block_id) {
    return &cache[cache_sync_sector(block_id, true)];
}

/* Part of keeping track of eviction policies. */
void cache_refresh(void) {
	cache_refresh_count++;
	if(cache_refresh_count % CACHE_REFRESH_LIMIT == 0) {
		int i;

		/* Delay the reset of the count so the decay isn't so severe */
		if(cache_refresh_count > CACHE_REFRESH_LIMIT * CACHE_DELAY_MULTIPLIER) {
			cache_refresh_count = 0;
			cache_write_all_dirty();
		}

		/* Take all accessed bits and set them back to false. If it was
		accessed recently, add that fact to our counter. */
		for(i = 0; i < CACHE_SIZE; i++) {
			/* Add decay so things accessed long ago will not stay around
			in the cache. Only resets when refresh count is 0. */
			if(cache_access_count[i] > 0 && cache_refresh_count == 0) {
				cache_access_count[i] -= 1;


/*	printf("USED:");
	for (i = 0; i < CACHE_SIZE; ++i) {
		printf("%d ", cache[i].used);
	}
	printf("\n");

	printf("ACCCOUNT:");
	for (i = 0; i < CACHE_SIZE; ++i) {
		printf("%d ", cache_access_count[i]);
	}
	printf("\n");
*/
			}

			if(cache[i].used && cache[i].accessed) {
				cache_access_count[i]++;
				cache[i].accessed = false;
			}
		}
	}
}
