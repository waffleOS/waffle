#ifndef VM_FRAME_TABLE_H 
#define VM_FRAME_TABLE_H 
#include "vm/structs.h"

/* General initialization of module. */
void init_frame_table(void);

/* Allocate a frame for a page. */
struct frame *falloc(struct page_info *p);

#endif /* VM_FRAME_TABLE_H */
