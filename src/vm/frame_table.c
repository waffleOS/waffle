#include <hash.h>
#include "threads/synch.h"
#include "vm/frame_table.h"

/* Variables */
struct semaphore frame_table_sem;

void init_frame_table(void) { 
    /* Initialize semaphore to control hash table. */
    sema_init(&frame_table_sem, 1);
}

