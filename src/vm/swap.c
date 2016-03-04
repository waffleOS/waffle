#include <debug.h>
#include "bitmap.h"
#include "devices/block.h"
#include "vm/swap.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"

/* Static variables */
/* Bitmap to manage free slots. */
static struct bitmap *swap_bitmap;
/* Actual swap device. */
static struct block *swap;
/* Mutex to control swap device. */
static struct semaphore swap_sem;
/* Mutex to control bitmap. */
static struct semaphore bm_sem;

/* Initialize swap module */
void init_swap() {
    swap = block_get_role(BLOCK_SWAP);
    swap_bitmap = bitmap_create(block_size(swap));
    sema_init(&swap_sem, 1);
    sema_init(&bm_sem, 1);
}

/* Moves a frame's page into a swap slot. */
void save_frame_page(struct frame *f) {
    size_t index;
    struct page_info *pinfo = f->pinfo;

    //printf("Saving data at kpage %p upage %p to swap\n", f->addr, pinfo->upage);
    
    /* Obtain index of swap slot to use. */
    sema_down(&bm_sem);
    index = bitmap_scan_and_flip(swap_bitmap, 0, 1, false);
    sema_up(&bm_sem);

    /* Panic if no space available. */
    if (index == BITMAP_ERROR) { 
        PANIC("Error: out of swap slots.\n");
    }
    /* Otherwise move page to swap slot. */
    else { 
        pinfo->status = SWAP;
        f->pinfo = NULL;
        pinfo->swap_index = index;
        struct thread *t = thread_current();

        /* Atomically write to block. */
        sema_down(&swap_sem);
        block_write(swap, index, pinfo->upage);
        sema_up(&swap_sem);
        
        pagedir_clear_page(&t->pagedir, pinfo->upage);
    }
}

/* Restores a page from a swap slot. */
void restore_page(struct page_info *p) {
    size_t index = p->swap_index;

    //printf("Restoring data at upage %p from swap\n", p->upage);
    sema_down(&swap_sem);
    block_read(swap, index, p->upage);
    sema_up(&swap_sem);
    
    /* Allow to be used by other frames. */
    sema_down(&bm_sem);
    bitmap_reset(swap_bitmap, index);
    sema_up(&bm_sem);
}
