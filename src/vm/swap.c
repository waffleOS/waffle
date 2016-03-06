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
    if (pinfo == NULL)
    {
        /*printf("I am here\n");*/
        return;
    }

    /*printf("Saving data at kpage %p upage %p to swap\n", f->addr, pinfo->upage);*/
    
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
        /*printf("Writing page to swap\n");*/

        pinfo->status = SWAP;
        f->pinfo = NULL;
        pinfo->swap_index = index;

        /* Atomically write to block. */
        if (pagedir_is_dirty(pinfo->pagedir, pinfo->upage))
        {
            /*printf("Page is dirty\n");*/
            sema_down(&swap_sem);
            /*printf("Sema down swap\n");*/
            /*printf("Upage: %p\n", pinfo->upage);*/
            int i;
            void * addr = f->addr;
            for (i = 0; i < PGSIZE / BLOCK_SECTOR_SIZE; i++)
            {
                /*printf("Writing to swap, iter %d\n", i);*/
                block_write(swap, index + i, addr);
                addr += BLOCK_SECTOR_SIZE;
            }
            /*printf("Wrote page to swap\n");*/
            sema_up(&swap_sem);

        }
        pagedir_clear_page(pinfo->pagedir, pinfo->upage);
        
    }
}

/* Restores a page from a swap slot. */
void restore_page(struct page_info *p) {
    size_t index = p->swap_index;

    /*printf("Restoring data at upage %p from swap\n", p->upage);*/
    sema_down(&swap_sem);
    int i;
    void * addr = p->frame->addr;
    for (i = 0; i < PGSIZE / BLOCK_SECTOR_SIZE; i++)
    {
        /*printf("Writing to swap, iter %d\n", i);*/
        block_read(swap, index + i, addr);
        addr += BLOCK_SECTOR_SIZE;
    }
    sema_up(&swap_sem);
    
    /* Allow to be used by other frames. */
    sema_down(&bm_sem);
    bitmap_reset(swap_bitmap, index);
    sema_up(&bm_sem);
}
