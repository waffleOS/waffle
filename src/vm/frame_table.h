#ifndef VM_FRAME_TABLE_H 
#define VM_FRAME_TABLE_H 
#include "vm/structs.h"

/* General initialization of module. */
void init_frame_table(void);

/* Eviction strategy */ 
struct page *evict_frame(void);

#endif /* VM_FRAME_TABLE_H */
