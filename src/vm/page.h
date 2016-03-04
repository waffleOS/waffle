#ifndef VM_PAGE_H
#define VM_PAGE_H
#include "vm/structs.h"
#include <debug.h>

/* Public API functions */
/* Constructs a page_info and adds it to the hash table. */
struct page_info * install_page_info(uint8_t * upage, struct file * file, off_t ofs, uint32_t read_bytes, uint32_t zero_bytes, bool writable, enum page_status status);
/* Initialize module. */
void init_page_info_hash(struct hash * supplemental_page_table);
/* Find page info for faulting page. */
struct page_info * page_info_lookup(struct hash * sup_page_table, const uint8_t * upage);
/* Process clean up. */
void clean_up_sup_page_table(void);


#endif /* VM_PAGE_H */
