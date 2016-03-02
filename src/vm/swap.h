#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "vm/structs.h"

void init_swap(void);
void save_frame_page(struct frame *f);
void restore_page(struct page_info *p);

#endif /* VM_SWAP_H */
