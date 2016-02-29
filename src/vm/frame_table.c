#include <debug.h>
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "vm/frame_table.h"

/* Variables */
/* Semaphore for frame table. */
static struct semaphore frame_table_sem;
/* Semaphore for free list. */
static struct semaphore free_sem;
/* List to store active frames. */
static struct list frame_table;
/* List to store free frames (after a process terminates). */
static struct list free_frames;


/* Module helper function prototypes. */
struct frame *get_free_frame(void);
struct frame *evict_frame(void); 

/* Module initialization. */
void init_frame_table(void) { 
    /* Initialize semaphore to control frame table. */
    sema_init(&frame_table_sem, 1);
    sema_init(&free_sem, 1);

    /* Initialize lists of frames. */
    list_init(&frame_table);
    list_init(&free_frames);
}

/**
 * Allocates a frame for a user page. First checks for
 * any available frames in the frame table. If not,
 * calls palloc_get_page() to try to get a page.
 * If one is not available, evicts a frame, and 
 * stores it in swap.
 */
struct frame *falloc(struct page_info *p) { 
    /* Try to look for free frame in table */
    struct frame *f = get_free_frame();
    void *addr;

    /* Add to frame_table if there is a free frame. */
    if (f != NULL) { 
        /* Synchronously add to frame_table. */
        sema_down(&frame_table_sem);
        list_push_back(&frame_table, &f->elem);
        sema_up(&frame_table_sem);
    }
    /* If no free frame, try to palloc. */
    else { 
        addr = palloc_get_page(PAL_USER);
        /* If addr != NULL, we got the page */
        if (addr != NULL) {
            f = malloc(sizeof(struct frame));
            f->addr = addr;
            f->pinfo = p;
        }
        /* Otherwise we need to try to evict. */
        else { 
            /* If no swap space, kernel panic. */
            f = evict_frame();
        }
    }

    return f;
}

/**
 * Puts the frame in the free_frames list, which is the
 * first thing we check when calling falloc to allocate
 * a frame.
 */
void free_frame(struct frame *f) { 
    
    /* Synchronously remove from frame_table. */
    sema_down(&frame_table_sem);
    list_remove(&f->elem);
    sema_up(&frame_table_sem);

    /* Syncrhonously add to free_frames. */
    sema_down(&free_sem);
    list_push_back(&free_frames, &f->elem);
    sema_up(&free_sem);
}

/**
 * Checks the free_frames list for the first free frame.
 * Returns NULL if there is no free frame.
 */
struct frame *get_free_frame() { 

    /* Obtain access to free_list. */
    sema_down(&free_sem);

    struct frame *f = NULL;
    /* Return NULL if no free frame. */
    if (!list_empty(&free_frames)) {
        f = list_entry(list_pop_front(&free_frames), 
                struct frame, elem);
    } 

    /* Give up access to free_list. */
    sema_up(&free_sem);

    return f;
}

/**
 * Chooses and evicts a frame.
 */
struct frame *evict_frame() { 
    PANIC("Frame eviction failed.");
    return NULL;
}

