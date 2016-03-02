#include <stdbool.h>
#include <stdint.h>
#include "vm/page.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include <hash.h>
#include "threads/vaddr.h"

/* Macros */
#define NUM_PAGES = 1048576 /* = 2 ^ 20 bits per page number */

/* Variables */


/* Installs page info in the supplemental page table */
struct page_info * install_page_info(uint8_t * upage, struct file * file, off_t ofs, uint32_t read_bytes, uint32_t zero_bytes, bool writable, enum page_status status)
{
    struct page_info * p_info = malloc(sizeof(struct page_info));
    p_info->page_num = pg_no(upage);
    p_info->upage = upage;
    p_info->file = file;
    p_info->ofs = ofs;
    p_info->read_bytes = read_bytes;
    p_info->zero_bytes = zero_bytes;
    p_info->writable = writable;
    p_info->status = status;

    struct thread * t = thread_current();
    hash_insert(&t->sup_page_table, &p_info->elem);
    return p_info == NULL ? NULL : p_info;
}

unsigned int page_info_hash(const struct hash_elem *p_, void *aux UNUSED)
{
    const struct page_info * p = hash_entry(p_, struct page_info, elem);
    return hash_bytes(&p->page_num, sizeof p->page_num);
}

bool page_info_less(const struct hash_elem * a_, const struct hash_elem * b_, void * aux UNUSED)
{
    const struct page_info * a = hash_entry(a_, struct page_info, elem);
    const struct page_info * b = hash_entry(b_, struct page_info, elem);

    return a->page_num < b->page_num;
}

void init_page_info_hash(struct hash * supplemental_page_table)
{
    hash_init(supplemental_page_table, page_info_hash, page_info_less, NULL);
}


struct page_info * page_info_lookup(struct hash * sup_page_table, const uint8_t * upage)
{
    struct page_info p_info;
    struct hash_elem * e;

    p_info.page_num = pg_no(upage);
    e = hash_find(sup_page_table, &p_info.elem);
    return e != NULL ? hash_entry(e, struct page_info, elem) : NULL;
}

struct page_info * page_info_delete(struct hash * sup_page_table, const uint8_t * upage)
{
    struct page_info p_info;
    struct hash_elem * e;

    p_info.page_num = pg_no(upage);
    e = hash_delete(sup_page_table, &p_info.elem);
    return e != NULL ? hash_entry(e, struct page_info, elem) : NULL;
}
