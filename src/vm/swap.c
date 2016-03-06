#include <debug.h>
#include "bitmap.h"
#include "devices/block.h"
#include "vm/swap.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"

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

    /* Check if the page_info for the current frame is NULL. This should never
     * happen since we only evict frames with active threads */
    if (pinfo == NULL)
    {
        return;
    }

    
    /* Obtain index of swap slot to use. */
    sema_down(&bm_sem);
    index = bitmap_scan_and_flip(swap_bitmap, 0, 8, false);
    sema_up(&bm_sem);

    /* Panic if no space available. */
    if (index == BITMAP_ERROR) { 
        PANIC("Error: out of swap slots.\n");
    }
    /* Otherwise move page to swap slot. */
    else { 
        struct thread *t = thread_current();

        /* Update the status of the page_info */
        pinfo->status = SWAP;
        f->pinfo = NULL;
        pinfo->swap_index = index;

        /* Atomically write to block if the page is dirty. */
        if (pagedir_is_dirty(pinfo->pagedir, pinfo->upage))
        {
            sema_down(&swap_sem);

            /* Iterate through the page and write BLOCK_SECTOR_SIZE bytes to
             * swap each time */
            int i;
            void * addr = f->addr;
            for (i = 0; i < PGSIZE / BLOCK_SECTOR_SIZE; i++)
            {
                block_write(swap, index + i, addr);
                addr += BLOCK_SECTOR_SIZE;
            }
            sema_up(&swap_sem);

        }

        /* Clear the page in the pagedir of the thread owning the page */
        pagedir_clear_page(pinfo->pagedir, pinfo->upage);
        
    }
}

/* Restores a page from a swap slot. */
void restore_page(struct page_info *p) {
    size_t index = p->swap_index;

    sema_down(&swap_sem);

    /* Iterate through the page and read BLOCK_SECTOR_SIZE bytes to
     * swap each time */
    int i;
    void * addr = p->frame->addr;
    for (i = 0; i < PGSIZE / BLOCK_SECTOR_SIZE; i++)
    {
        block_read(swap, index + i, addr);
        addr += BLOCK_SECTOR_SIZE;
    }
    sema_up(&swap_sem);
    
    /* Allow to be used by other frames. */
    sema_down(&bm_sem);
    bitmap_reset(swap_bitmap, index);
    sema_up(&bm_sem);
}
