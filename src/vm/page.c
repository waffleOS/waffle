#include <stdbool.h>
#include "vm/page.h"
#include "threads/malloc.h"
#include <hash.h>

/* Macros */
#define NUM_PAGES = 1048576 /* = 2 ^ 20 bits per page number */

/* Variables */


/* Installs page info in the supplemental page table */
bool install_page_info(uint8_t * upage, struct file * file, off_t ofs, uint32_t read_bytes, uint32_t zero_bytes, bool writable, enum page_status status)
{
   struct page_info * p_info = malloc(sizeof(struct page_info));
   p_info->upage = upage;
   p_info->file = file;
   p_info->ofs = ofs;
   p_info->read_bytes = read_bytes;
   p_info->zero_bytes = zero_bytes;
   p_info->writable = writable;
   p_info->status = status;
}

unsigned int page_info_hash(const struct hash_elem *p_, void *aux UNUSED)
{
    const struct page_info * p = hash_entry(p_, struct page_info, elem);
    return hash_bytes(&p->upage, sizeof p->upage);
}

bool page_info_less(const struct hash_elem * a_, const struct hash_elem * b_, void * aux UNUSED)
{
    const struct page_info * a = hash_entry(a_, struct page_info, elem);
    const struct page_info * b = hash_entry(b_, struct page_info, elem);

    return a->upage < b->upage;
}

void init_page_info_hash(struct hash * supplemental_page_table)
{
    hash_init(supplemental_page_table, page_info_hash, page_info_less, NULL);
}
