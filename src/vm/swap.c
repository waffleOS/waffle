#include <debug.h>
#include "bitmap.h"
#include "devices/block.h"
#include "vm/swap.h"
#include "threads/synch.h"

/* Static variables */
static struct bitmap *swap_bitmap;
static struct block *swap;
static struct semaphore swap_sem;
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

        /* Atomically write to block. */
        sema_down(&swap_sem);
        block_write(swap, index, pinfo->upage);
        sema_up(&swap_sem);
    }
}

/* Restores a page from a swap slot. */
void restore_page(struct page_info *p) {
}
