#include <hash.h>
#include <debug.h>
#include "threads/synch.h"
#include "vm/frame_table.h"

/* Variables */
static struct semaphore frame_table_sem;
/* Hash table to store active frames. */
static struct hash frame_table;
/* List to store free frames (after a process terminates). */
static struct list free_frames;


/* Module helper function prototypes. */
struct frame *get_free_frame(void);
struct frame *evict_frame(void); 

/* Hash table functions */
unsigned frame_hash (const struct hash_elem *p_, void *aux UNUSED);
bool frame_less (const struct hash_elem *a_, const struct hash_elem *b_,
                void *aux UNUSED);

/* Module initialization. */
void init_frame_table(void) { 
    /* Initialize semaphore to control hash table. */
    sema_init(&frame_table_sem, 1);
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

    /* If no free frame, try to palloc. */
    if (f == NULL) { 
        addr = palloc_get_page(PAL_USER);
        /* If addr != NULL, we got the page */
        if (addr != NULL) {
        /* Otherwise we need to try to evict. */
        else { 
        }
    
        /* If no swap space, kernel panic. */
    }

    

    return f;
}

/**
 * Puts the frame in the free_frames list, which is the
 * first thing we check when calling falloc to allocate
 * a frame.
 */
void free_frame(struct frame *f) { 
}

/**
 * Checks the free_frames list for the first free frame.
 * Returns NULL if there is no free frame.
 */
struct frame *get_free_frame() { 
}

/**
 * Chooses and evicts a frame.
 */
struct frame *evict_frame() { 

}

    
/* Returns a hash value for frame f. */
unsigned frame_hash (const struct hash_elem *f_, void *aux UNUSED) {
  const struct frame *f = hash_entry (f_, struct frame, elem);
  return hash_bytes (&f->addr, sizeof f->addr);
}

/* Returns true if frame a precedes frame b. */
bool frame_less(const struct hash_elem *a_, const struct hash_elem *b_,
                void *aux UNUSED)
{
  const struct frame *a = hash_entry (a_, struct frame, elem);
  const struct frame *b = hash_entry (b_, struct frame, elem);

  return a->addr < b->addr;
}
