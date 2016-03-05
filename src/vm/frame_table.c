#include <debug.h>
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "vm/frame_table.h"
#include "vm/swap.h"

#define INIT_AGE UINT32_MAX

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
    /*printf("Trying to get frame for: %p\n", p->upage);*/

    /* Add to frame_table if there is a free frame. */
    if (f != NULL) { 
        f->pinfo = p;
        f->age = INIT_AGE;

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
            /* Allocate new frame struct. */ 
            f = malloc(sizeof(struct frame));
            f->addr = addr;
            f->pinfo = p;
            f->age = INIT_AGE;

            /* Synchronously add to frame_table. */
            sema_down(&frame_table_sem);
            list_push_back(&frame_table, &f->elem);
            sema_up(&frame_table_sem);

        }
        /* Otherwise we need to try to evict. */
        else { 
            f = evict_frame();
            
            /* If not mmaped file, add to swap.  */
            /*if (f->pinfo != NULL && f->pinfo->status != MMAP_FILE) { */
                /* If no swap space, kernel panic. */
                save_frame_page(f);
            /*}*/
            /*else {*/
                /* TODO: write back to mmaped file. */
            /*}*/

            f->pinfo = p;
            f->age = INIT_AGE;
        }
    }
    /*printf("Got frame!\n");*/

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
 * Chooses a frame to evict. Sets up the frame table, and
 * frame so the new page can be added. The old page should
 * be saved before setting the new page.
 */
struct frame *evict_frame() { 
    /* Obtain lock before choosing which frame to evict. */
    sema_down(&frame_table_sem);

    /* Frame table should not be empty if we need to evict. */
    if (list_empty(&frame_table)) { 
        PANIC("Frame eviction failed.");
        return NULL;
    }

    /** 
     * Use age policy and find youngest frame, which
     * is the frame accessed least recently up to the
     * resolution of age, which is 32 bit.
     */
    struct list_elem *cur = list_begin(&frame_table);
    struct frame *f = list_entry(cur, struct frame, elem);
    uint32_t age = f->age;

    for (cur = list_next(cur); cur != list_end(&frame_table);
         cur = list_next(cur)) { 
        struct frame *cur_frame = list_entry(cur, struct frame, elem);
        if (cur_frame->age <= age) { 
            age = cur_frame->age;
            f = cur_frame;
        }
    }

    //printf("Evicting kpage: %p, upage %p\n", f->addr, f->pinfo->upage);
    
    /** 
     * Currently removes the selected frame, and puts it back to the
     * front because the aging implementation is broken. This will
     * be unnecessary when aging is debugged.
     */
    list_remove(&f->elem);
    list_push_front(&frame_table, &f->elem);

    sema_up(&frame_table_sem);

    return f;
}

/* Update all frame ages. */
void update_frame_ages() { 
    struct list_elem *cur;

    /** 
     * Commented out aging eviction code:
     * Breaks tests/vm/mmap-inherit and tests/vm/page-linear
     * but works for all Project 4 regression tests, and most
     * of the other tests.
     *
    for (cur = list_begin(&frame_table); cur != list_end(&frame_table);
         cur = list_next(cur)) { 
        struct frame *f = list_entry(cur, struct frame, elem);
        struct thread *t = thread_current();
        f->age = (f->age >> 1) & 
            (pagedir_is_accessed(&t->pagedir, f->pinfo->upage) << 31);
        pagedir_set_accessed(&t->pagedir, f->pinfo->upage, false);
    }
    */
}

