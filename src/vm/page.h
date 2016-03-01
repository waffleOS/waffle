#ifndef VM_PAGE_H
#define VM_PAGE_H
#include "vm/structs.h"
#include <debug.h>

bool install_page_info(uint8_t * upage, struct file * file, off_t ofs, uint32_t read_bytes, uint32_t zero_bytes, bool writable, enum page_status status);
unsigned int page_info_hash(const struct hash_elem *p_, void *aux UNUSED);
bool page_info_less(const struct hash_elem * a_, const struct hash_elem * b_, void * aux UNUSED);
void init_page_info_hash(struct hash * supplemental_page_table);



#endif /* VM_PAGE_H */
