#ifndef VM_FRAME_TABLE_H 
#define VM_FRAME_TABLE_H 
#include "vm/structs.h"

/* General initialization of module. */
void init_frame_table(void);

/* Allocate a frame for a page. */
struct frame *falloc(struct page_info *p);

/* Free a frame so we know it is available. */
void free_frame(struct frame *f);

/* */
void update_frame_ages(void);

#endif /* VM_FRAME_TABLE_H */
