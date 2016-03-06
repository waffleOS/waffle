#include <stdbool.h>
#include <stdint.h>
#include "vm/page.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include <hash.h>
#include "threads/vaddr.h"

/* Macros */
#define NUM_PAGES = 1048576 /* = 2 ^ 20 bits per page number */

/* Static variables */

/* Module Internal functions. */
unsigned int page_info_hash(const struct hash_elem *p_, void *aux UNUSED);
bool page_info_less(const struct hash_elem * a_, const struct hash_elem * b_, void * aux UNUSED);
void clean_up_elem(struct hash_elem *e, void *aux UNUSED);




/* Installs page info in the supplemental page table */
struct page_info * install_page_info(uint8_t * upage, struct file * file, off_t ofs, uint32_t read_bytes, uint32_t zero_bytes, bool writable, enum page_status status)
{
    // Allocate dynamic memory for the page_info
    struct page_info * p_info = malloc(sizeof(struct page_info));

    // Set the page number to the page number of the given address. This is used as a key in the hash table
    p_info->page_num = pg_no(upage);
    p_info->upage = upage;
    p_info->file = file;
    p_info->ofs = ofs;
    p_info->read_bytes = read_bytes;
    p_info->zero_bytes = zero_bytes;
    p_info->writable = writable;
    p_info->status = status;

    // The page_info does not have a frame yet
    p_info->frame = NULL;

    struct thread * t = thread_current();

    // Set the pagedir to the current thread's pagedir. This is used later for eviction
    p_info->pagedir = t->pagedir;
    
    // Insert the page_info in the hash table
    bool success = hash_insert(&t->sup_page_table, &p_info->elem) == NULL;

    // Return the page_info if the insertion was successful, otherwise return NULL
    return success ? p_info : NULL;
}

/* Hash function for hash table of supplemental page table */
unsigned int page_info_hash(const struct hash_elem *p_, void *aux UNUSED)
{
    const struct page_info * p = hash_entry(p_, struct page_info, elem);
    return hash_bytes(&p->page_num, sizeof p->page_num);
}

/* Comparator function for hash table of supplemental page table */
bool page_info_less(const struct hash_elem * a_, const struct hash_elem * b_, void * aux UNUSED)
{
    const struct page_info * a = hash_entry(a_, struct page_info, elem);
    const struct page_info * b = hash_entry(b_, struct page_info, elem);

    return a->page_num < b->page_num;
}

/* Initializes a supplemental page table */
void init_page_info_hash(struct hash * supplemental_page_table)
{
    hash_init(supplemental_page_table, page_info_hash, page_info_less, NULL);
}


/* Looks up the page_info corresponding to the page number of the given address in the
 * supplemental page table. Returns the page_info if it was found, otherwise returns NULL */
struct page_info * page_info_lookup(struct hash * sup_page_table, const uint8_t * upage)
{
    struct page_info p_info;
    struct hash_elem * e;

    p_info.page_num = pg_no(upage);
    e = hash_find(sup_page_table, &p_info.elem);
    return e != NULL ? hash_entry(e, struct page_info, elem) : NULL;
}

/* Deletes the page_info corresponding to the page number of the given address
 * from the supplemental page table. Returns the page_info if found, otherwise
 * returns NULL */
struct page_info * page_info_delete(struct hash * sup_page_table, const uint8_t * upage)
{
    struct page_info p_info;
    struct hash_elem * e;

    p_info.page_num = pg_no(upage);
    e = hash_delete(sup_page_table, &p_info.elem);
    return e != NULL ? hash_entry(e, struct page_info, elem) : NULL;
}

/**
 * Cleans up supplemental page table of current thread.
 * Should be called when a process is being cleaned up 
 * in pagedir_destroy().
 */
void clean_up_sup_page_table(void) {
    struct thread *t = thread_current();
    struct hash *spt = &t->sup_page_table;
    hash_destroy(spt, clean_up_elem);
    
}     

/* Helper function for cleaning up supplemental page table */
void clean_up_elem(struct hash_elem *e, void *aux UNUSED) {
    struct page_info *p = hash_entry(e, struct page_info, elem);

    // Free the frame of the page_info if it exists
    if(p->frame != NULL)
    {
        free_frame(p->frame);
    }

    free(p);
}
